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

#include <libaegisub/signal.h>

#include "main.h"

using namespace agi::signal;

struct increment {
	int *num;
	increment(int &num) : num(&num) { }
	void operator()() const { ++*num; }
	void operator()(int n) const { *num += n; }
};

TEST(lagi_signal, basic) {
	Signal<> s;
	int x = 0;
	Connection c = s.Connect(increment(x));

	EXPECT_EQ(0, x);
	s();
	EXPECT_EQ(1, x);
}

TEST(lagi_signal, multiple) {
	Signal<> s;
	int x = 0;
	Connection c1 = s.Connect(increment(x));
	Connection c2 = s.Connect(increment(x));

	EXPECT_EQ(0, x);
	s();
	EXPECT_EQ(2, x);
}
TEST(lagi_signal, manual_disconnect) {
	Signal<> s;
	int x = 0;
	Connection c1 = s.Connect(increment(x));
	EXPECT_EQ(0, x);
	s();
	EXPECT_EQ(1, x);
	c1.Disconnect();
	s();
	EXPECT_EQ(1, x);
}

TEST(lagi_signal, auto_disconnect) {
	Signal<> s;
	int x = 0;

	EXPECT_EQ(0, x);
	{
		Connection c = s.Connect(increment(x));
		s();
		EXPECT_EQ(1, x);
	}
	s();
	EXPECT_EQ(1, x);
}

TEST(lagi_signal, connection_outlives_slot) {
	int x = 0;
	Connection c;

	EXPECT_EQ(0, x);
	{
		Signal<> s;
		c = s.Connect(increment(x));
		s();
		EXPECT_EQ(1, x);
	}
	c.Disconnect();
}

TEST(lagi_signal, one_arg) {
	Signal<int> s;
	int x = 0;

	Connection c = s.Connect(increment(x));
	s(0);
	EXPECT_EQ(0, x);
	s(10);
	EXPECT_EQ(10, x);
	s(20);
	EXPECT_EQ(30, x);
}

TEST(lagi_signal, two_args) {
	Signal<int, int> s;
	int x = 0, y = 0;

	Connection c = s.Connect([&](int a, int b) { x = a; y = b; });
	s(0, 5);
	EXPECT_EQ(0, x);
	EXPECT_EQ(5, y);
	s(10, 15);
	EXPECT_EQ(10, x);
	EXPECT_EQ(15, y);
}

class uncopyable {
	uncopyable(uncopyable const&) = delete;
	uncopyable(uncopyable&&) = delete;
	void operator=(uncopyable&&) = delete;
	void operator=(uncopyable const&) = delete;
public:
	int value;
};

TEST(lagi_signal, uncopyable_const_ref) {
	Signal<uncopyable const&> s;
	const uncopyable value = {10};
	Connection c = s.Connect([](uncopyable const& u) {
		EXPECT_EQ(u.value, 10);
	});
	s(value);
}

TEST(lagi_signal, uncopyable_ref) {
	Signal<uncopyable&> s;
	uncopyable value = {10};
	Connection c = s.Connect([](uncopyable& u) {
		EXPECT_EQ(10, u.value);
	});
	s(value);
}

TEST(lagi_replay_signal, should_send_initial_value_on_connection) {
	ReplaySignal<int> s(5);
	int x = 0;
	Connection c = s.Connect([&](int value) { x = value; });
	EXPECT_EQ(5, x);
}

TEST(lagi_replay_signal, should_send_last_sent_value) {
	ReplaySignal<int> s(5);
	int x = 0;
	s(10);
	Connection c = s.Connect([&](int value) { x = value; });
	EXPECT_EQ(10, x);
}

TEST(lagi_replay_signal, should_support_multiple_values) {
	ReplaySignal<int, int, int> s(1, 2, 3);
	int x = 0, y = 0, z = 0;
	Connection c = s.Connect([&](int a, int b, int c) { x = a; y = b; z = c; });
	EXPECT_EQ(1, x);
	EXPECT_EQ(2, y);
	EXPECT_EQ(3, z);
}
