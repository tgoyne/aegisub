// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

/// @file signal.h
/// @brief 
/// @ingroup libaegisub

#pragma once

#include <boost/container/map.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

#include <functional>
#include <memory>

namespace agi {
	namespace signal {

using namespace std::placeholders;

class Connection;

/// Implementation details; nothing outside this file should directly touch
/// anything in the detail namespace
namespace detail {
	class SignalBase;
	class ConnectionToken {
		friend class agi::signal::Connection;
		friend class SignalBase;

		SignalBase *signal;
		bool blocked;
		bool claimed;

		ConnectionToken(SignalBase *signal) : signal(signal), blocked(false), claimed(false) { }
		inline void Disconnect();
	public:
		~ConnectionToken() { Disconnect(); }
	};
}

/// @class Connection
/// @brief Object which owns a connection between a signal and a slot
class Connection {
	std::unique_ptr<detail::ConnectionToken> token;
public:
	Connection() { }
	Connection(Connection&& that) : token(std::move(that.token)) { }
	Connection(detail::ConnectionToken *token) : token(token) { token->claimed = true; }
	Connection& operator=(Connection&& that) { token = std::move(that.token); return *this; }

	/// @brief End this connection
	///
	/// This normally does not need to be manually called, as a connection is
	/// automatically closed when all Connection objects referring to it are
	/// gone. To temporarily enable or disable a connection, use Block/Unblock
	/// instead
	void Disconnect() { if (token) token->Disconnect(); }

	/// Disable this connection until Unblock is called
	void Block() { if (token) token->blocked = true; }

	/// Reenable this connection after it was disabled by Block
	void Unblock() { if (token) token->blocked = false; }
};
/// @class UnscopedConnection
/// @brief A connection which is not automatically closed
///
/// Connections initially start out owned by the signal. If a slot knows that
/// it will outlive a signal and does not need to be able to block a
/// connection, it can simply ignore the return value of Connect.
///
/// If a slot needs to be able to disconnect from a signal, it should store the
/// returned connection in a Connection, which transfers ownership of the
/// connection to the slot. If there is any chance that the signal will outlive
/// the slot, this must be done.
class UnscopedConnection {
	detail::ConnectionToken *token;
public:
	UnscopedConnection(detail::ConnectionToken *token) : token(token) { }
	operator Connection() { return Connection(token); }
};

namespace detail {
	/// @brief Polymorphic base class for signals
	///
	/// This class is used to avoid having to make Connection templated on the
	/// signal type, by giving it a virtual Disconnect function to call. It
	/// also speeds up compilation slightly by extracting as much as possible
	/// from the templated implementation.
	class SignalBase {
		friend class ConnectionToken;
		/// @brief Disconnect the passed slot from the signal
		/// @param tok Token to disconnect
		virtual void Disconnect(ConnectionToken *tok)=0;

		/// Signals can't be copied
		SignalBase(SignalBase const&);
		SignalBase& operator=(SignalBase const&);
	protected:
		SignalBase() { }

		/// @brief Notify a slot that it has been disconnected
		/// @param tok Token to disconnect
		///
		/// Used by the signal when the signal wishes to end a connection (such
		/// as if the signal is being destroyed while slots are still connected
		/// to it)
		void DisconnectToken(ConnectionToken *tok) { tok->signal = nullptr; }

		/// Has a token been claimed by a scoped connection object?
		bool TokenClaimed(ConnectionToken *tok) { return tok->claimed; }

		/// Create a new token that owns a connection to this signal
		ConnectionToken *MakeToken() { return new ConnectionToken(this); }

		/// Check if a slot currently wants to receive signals
		bool Blocked(ConnectionToken *tok) { return tok->blocked; }
	};

	inline void ConnectionToken::Disconnect() {
		if (signal) signal->Disconnect(this);
		signal = nullptr;
	}

	template<typename Derived, typename... Args>
	class SignalImpl : public SignalBase {
	protected:
		typedef std::function<void (Args...)> Slot;
		typedef boost::container::map<ConnectionToken*, Slot> SlotMap;

		SlotMap slots; /// Slots currently connected to this signal

		void Disconnect(ConnectionToken *tok) override {
			slots.erase(tok);
		}

		~SignalImpl() {
			for (auto& slot : slots) {
				DisconnectToken(slot.first);
				if (!TokenClaimed(slot.first)) delete slot.first;
			}
		}

	protected:
		UnscopedConnection DoConnect(Slot sig) {
			auto token = MakeToken();
			slots.insert(std::make_pair(token, sig));
			return UnscopedConnection(token);
		}

		template<typename... FnArgs>
		void SendNext(FnArgs&&... args) {
			for (auto cur = slots.begin(); cur != slots.end(); ) {
				if (Blocked(cur->first))
					++cur;
				else
					(cur++)->second(std::forward<Args>(args)...);
			}
		}

	public:
		UnscopedConnection Connect(Slot sig) {
			return static_cast<Derived*>(this)->DoConnect(sig);
		}

		template<typename Func>
		UnscopedConnection Connect (Func&& fn) {
			return static_cast<Derived*>(this)->DoConnect(Slot(fn));
		}

		template<typename... FnArgs>
		UnscopedConnection Connect(FnArgs&&... args) {
			return static_cast<Derived*>(this)->DoConnect(std::bind(std::forward<FnArgs>(args)...));
		}

		template<typename T, typename Arg1>
		UnscopedConnection Connect(void (T::*func)(Arg1), T* self) {
			return static_cast<Derived*>(this)->DoConnect(std::bind(func, self), _1);
		}

		template<typename T, typename Arg1, typename Arg2>
		UnscopedConnection Connect(void (T::*func)(Arg1, Arg2), T* self) {
			return static_cast<Derived*>(this)->DoConnect(std::bind(func, self), _1, _2);
		}

		/// @brief Trigger this signal
		/// @param a1 The argument to the signal
		///
		/// The order in which connected slots are called is undefined and
		/// should not be relied on
		template<typename... FnArgs>
		void operator()(FnArgs&&... args) {
			static_cast<Derived*>(this)->SendNext(std::forward<FnArgs>(args)...);
		}
	};
}

template<typename... Args>
class Signal : public detail::SignalImpl<Signal<Args...>, Args...> { };

/// A signal which remembers the last sent values and sends it immediately to
/// any slots that connect to it
template<typename... Args>
class ReplaySignal : public detail::SignalImpl<ReplaySignal<Args...>, Args...> {
	typedef detail::SignalImpl<ReplaySignal<Args...>, Args...> super;

protected:
	std::tuple<Args...> value;

public:
	template<typename Func>
	UnscopedConnection DoConnect(Func&& fn) {
		auto connection = super::DoConnect(std::forward<Func>(fn));
		boost::fusion::invoke(fn, value);
		return connection;
	}

	template<typename... FnArgs>
	void SendNext(FnArgs&&... args) {
		value = std::make_tuple(args...);
		super::SendNext(std::forward<FnArgs>(args)...);
	}

public:
	ReplaySignal(Args... initial) : value(initial...) { }
};

/// A value with change notifications
template<typename T>
class Property : public ReplaySignal<T> {
	class Proxy;
	friend class Proxy;
	class Proxy {
		Property<T> *property;
	public:
		Proxy(Property<T> *property) : property(property) { }
		Proxy(Proxy&& proxy) : property(proxy.property) { proxy.property = nullptr; }
		~Proxy() {
			if (property)
				property->SendNext(std::get<0>(property->value));
		}
		T *operator->() const { return &std::get<0>(property->value); }
	};

	using ReplaySignal<T>::value;

public:
	typedef T value_type;

	Property(T value) : ReplaySignal<T>(value) { }
	operator T const&() const { return std::get<0>(value); }

	Property<T>& operator=(T const& new_value) {
		std::get<0>(value) = new_value;
		this->SendNext(new_value);
		return *this;
	}

	Property<T>& operator=(T&& new_value) {
		std::get<0>(value) = std::move(new_value);
		this->SendNext(std::get<0>(value));
		return *this;
	}

	Proxy Set() { return Proxy(this); }
	const T *operator->() const { return &std::get<0>(value); }
};

	}
}

/// @brief Define functions which forward their arguments to the connect method
///        of the named signal
/// @param sig Name of the signal
/// @param method Name of the connect method
///
/// When a signal is a member of a class, we typically want other objects to be
/// able to connect to the signal, but not to be able to trigger it. To do this,
/// make this signal private then use this macro to create public subscription
/// methods
///
/// Defines AddSignalNameListener
#define DEFINE_SIGNAL_ADDERS(sig, method) \
	template<class A>                         agi::signal::UnscopedConnection method(A a)             { return sig.Connect(a);       } \
	template<class A,class B>                 agi::signal::UnscopedConnection method(A a,B b)         { return sig.Connect(a,b);     } \
	template<class A,class B,class C>         agi::signal::UnscopedConnection method(A a,B b,C c)     { return sig.Connect(a,b,c);   } \
	template<class A,class B,class C,class D> agi::signal::UnscopedConnection method(A a,B b,C c,D d) { return sig.Connect(a,b,c,d); }
