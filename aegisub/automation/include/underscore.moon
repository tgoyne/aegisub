local table, ipairs, pairs, math, string = table, ipairs, pairs, math, string

local _ = {}
local chainable_mt = {}

_.identity = (value) -> value

_.reverse = (list) ->
  if _.isString(list)
    _(list).chain()\split()\reverse()\join()\value()
  else
    length = _.size list
    for i = 1, length / 2, 1
      list[i], list[length-i+1] = list[length-i+1], list[i]
    list

_.splice = (list, index, howMany, ...) ->
  return if not _.isArray list

  elements = {...}
  removed = {}

  howMany = howMany or #list - index + 1

  for i = 1, #elements, 1
    table.insert(list, i + index + howMany - 1, elements[i])

  for i = index, index + howMany - 1, 1
    table.insert(removed, table.remove(list, index))

  removed

_.pop = (list) ->
  table.remove list, #list

_.push = (list, ...) ->
  values = {...}
  _.each values, (v) -> table.insert list, v
  list

_.shift = (list) ->
  table.remove list, 1

_.unshift = (list, ...) ->
  values = {...}
  _.each _.reverse(values), (v) -> table.insert list, 1, v
  list

_.sort = (list, func) ->
  table.sort list, func or (a,b) -> tostring(a) < tostring(b)
  list

_.join = (list, separator) ->
  table.concat list, separator or ''

_.slice = (list, start, stop) ->
  array = {}
  stop = stop or #list

  table.insert array, list[index] for index = start, stop, 1

  array

_.concat = (...) ->
  values = _.flatten {...}, true
  cloned = {}

  _.each values, (v) -> table.insert cloned, v

  cloned

_.each = (list, func) ->
  pairing = if _.isArray list then ipairs else pairs

  for index, value in pairing(list)
    func value, index, list

_.map = (list, func) ->
  new_list = {}
  _.each list, (value, key, original_list) ->
    table.insert new_list, func value, key, original_list
  new_list

_.reduce = (list, func, memo) ->
  init = memo == nil
  _.each list, (value, key, object) ->
    if init
      memo = value
      init = false
    else
      memo = func(memo, value, key, object)

  if init
    error "Reduce of empty array with no initial value"

  memo

_.reduceRight = (list, func, memo) ->
  _.reduce _.reverse list, func, memo

_.find = (list, func) ->
  return if func == nil

  result = nil
  _.any list, (value, key, object) ->
    if func(value, key, object)
      result = value
      true

  result

_.select = (list, func) ->
  found = {}
  _.each list, (value, key, object) ->
    table.insert found, value if func(value, key, object)
  found

_.reject = (list, func) ->
  found = {}
  _.each list, (value, key, object) ->
    table.insert found, value unless func(value, key, object)
  found

_.all = (list, func) ->
  func = func or _.identity
  _.reduce list, (found, value, index, object) ->
    found and func value, index, object
  , true

_.any = (list, func) ->
  func = func or _.identity
  _.reduce list, (found, value, index, object) ->
    found or func value, index, object
  , false

_.include = (list, v) ->
  _.any list, (value) -> v == value

_.pluck = (list, key) ->
  _.map list, (value) -> value[key]

_.where = (list, properties) ->
  _.select list, (value) -> _.all properties, (v, k) -> value[k] == v

_.findWhere = (list, properties) ->
  _.first _.where list, properties

_.max = (list, func) ->
  return -math.huge if _.isEmpty list
  return math.max unpack list unless _.isFunction func

  max = computed: -math.huge
  _.each list, (value, key, object) ->
    computed = func(value, key, object)
    if computed >= max.computed
      max = computed: computed, value: value
  max.value

_.min = (list, func) ->
  return math.huge if _.isEmpty list
  return math.min unpack list unless _.isFunction func

  min = computed: math.huge
  _.each list, (value, key, object) ->
    computed = func value, key, object
    if computed < min.computed
      min = computed: computed, value: value
  min.value

_.invoke = (list, func, ...) ->
  invoke_func, args = func, {...}

  if _.isString(func)
    invoke_func = (value) -> value[func](value, unpack(args))

  _.collect list, (value) -> invoke_func value, unpack args

_.sortBy = (list, func) ->
  func = func or _.identity
  sorted_func = (a,b) -> a != nil and (b == nil or func(a) < func(b))

  if _.isString(func)
    sorted_func = (a,b) -> a[func](a) < b[func](b)

  table.sort(list, sorted_func)
  list

_.groupBy = (list, func) ->
  group_func, result = func, {}

  count_func = (v) -> v[func](v) if _.isString(func)

  _.each list, (value, key, object) ->
    key = group_func(value, key, object)
    result[key] = result[key] or {}
    table.insert result[key], value

  result

_.countBy = (list, func) ->
  count_func, result = func, {}

  count_func = (v) -> v[func](v) if _.isString(func)

  _.each list, (value, key, object) ->
    key = count_func(value, key, object)
    result[key] = (result[key] or 0) + 1

  result

_.shuffle = (list) ->
  rand, index, shuffled = 0, 1, {}
  _.each list, (value) ->
    rand = math.random(1, index)
    index += 1
    shuffled[index - 1] = shuffled[rand]
    shuffled[rand] = value

  shuffled

_.toArray = (list, ...) ->
  return {} unless list
  list = {list, ...} unless _.isObject list

  cloned = {}
  _.each list, (value) -> table.insert cloned, value
  cloned

_.size = (list, ...) ->
  local args = {...}

  if _.isArray(list)
    return #list
  elseif _.isObject(list)
    local length = 0
    _.each(list, () length = length + 1 end) ->
    return length
  elseif _.isString(list)
    return string.len(list)
  elseif not _.isEmpty(args)
    return _.size(args) + 1

  return 0

_.memoize = (func) ->
  list = {}
  (...) ->
    list[...] = func(...) unless list[...]
    list[...]

_.once = (func) ->
  called = false
  (...) ->
    if not called
      called = true
      func(...)

_.after = (times, func) ->
  return func() if times <= 0

  (...) ->
    times -= 1
    return func(...) if times < 1

_.bind = (func, context, ...) ->
  arguments = unpack {...}
  (...) -> func context, unpack _.concat arguments, ...

_.partial = (func, ...) ->
  args = {...}
  (self, ...) -> func self, _.concat args, {...}

_.wrap = (func, wrapper) ->
  (...) -> wrapper func, ...

_.compose = (...) ->
  funcs = {...}
  (...) ->
    args = {...}
    _.each _.reverse(funcs), (func) -> args = {func(unpack(args))}
    args[1]

_.range = (...) ->
  local args = {...}
  local start, stop, step = unpack(args)

  if #args <= 1
    stop = start or 0
    start = 0
  step = args[3] or 1

  local length, index, array =
    math.max(math.ceil((stop - start) / step), 0),
    0, {}

  while index < length
    table.insert(array, start)
    index += 1
    start += step

  return array

_.first = (list, count) ->
  return unless list
  if not count then list[1] else _.slice(list, 1, count)

_.rest = (list, start=2) ->
  _.slice(list, start, #list)

_.initial = (list, stop) ->
  _.slice(list, 1, stop or (#list - 1))

_.last = (list, count) ->
  return unless list

  if not count
    list[#list]
  else
    start, stop, array = #list - count + 1, #list, {}
    _.slice(list, start, stop)

_.compact = (list) ->
  _.filter list, (v) -> not not v

_.flatten = (list, shallow, output={}) ->
  _.each list, (value) ->
    if _.isArray value
      if shallow
        _.each value, (v) -> table.insert output, v
      else
        _.flatten value, false, output
    else
      table.insert output, value

  output

_.without = (list, ...) ->
  args = {...}
  _.difference(list, args)

_.uniq = (list, sorted, iterator) ->
  local initial, results, seen = list, {}, {}
  if iterator
    initial = _.map(list, iterator)

  _.each(initial, (value, index) ->
    if (sorted and (index==1 or seen[#seen]~=value)) or (not _.contains(seen, value))
      table.insert(seen, value)
      table.insert(results, list[index])
  end)

  return results

_.indexOf = (list, value, start=1) ->
  return 0 unless list

  for index = start, #list, 1
    return index if value == list[index]

  0

_.intersection = (a, ...) ->
  local b = {...}
  return _.filter(_.uniq(a), (item) ->
    return _.every(b, (other) ->
      return _.indexOf(other, item) >= 1
    end)
  end)

_.union = (...) ->
  return _.uniq(_.flatten({...}, true))

_.difference = (a, ...) ->
  local b = _.flatten({...}, true)
  return _.filter(a, (value) ->
    return not _.contains(b, value)
  end)

_.zip = (...) ->
  local args = {...}
  local length = _.max(_.map(args, (a) return #a end)) ->
  local results = {}

  for i=1, length, 1
    table.insert(results, _.pluck(args, i))

  return results

_.object = (list, values) ->
  if not list then return {} end

  local result = {}
  _.each(list, (value, index) ->
    if values
      result[value] = values[index]
    else
      result[value[1]] = value[2]
  end)

  return result

_.lastIndexOf = (list, value, start) ->
  if not list then return 0 end
  start = start or #list

  for index = start, 1, -1
    if value == list[index]
      return index

  return 0

_.keys = (list) ->
  if not _.isObject(list) then error("Not an object") end
  return _.map(list, (_, key) ->
    return key
  end)

_.values = (list) ->
  if _.isArray(list) then return list end
  return _.map(list, (value) ->
    return value
  end)

_.pairs = (list) ->
  return _.map(list, (value, key) ->
    return {key, value}
  end)

_.invert = (list) ->
  local array = {}

  _.each(list, (value, key) ->
    array[value] = key
  end)

  return array

_.functions = (list) ->
  local method_names = {}

  _.each(list, (value, key) ->
    if _.isFunction(value)
      table.insert(method_names, key)
  end)

  return method_names

_.extend = (list, ...) ->
  local lists = {...}
  _.each(lists, (source) ->
    _.each(source, (value, key) ->
      list[key] = source[key]
    end)
  end)

  return list

_.pick = (list, ...) ->
  local keys = _.flatten({...})

  local array = {}
  _.each(keys, (key) ->
    if list[key]
      array[key] = list[key]
  end)

  return array

_.omit = (list, ...) ->
  local keys = _.flatten({...})

  local array = {}
  _.each(list, (value,key) ->
    if not _.contains(keys, key)
      array[key] = list[key]
  end)

  return array

_.defaults = (list, ...) ->
  keys = {...}

  _.each(keys, (source) ->
    _.each(source, (value, key) ->
      list[key] = value unless list[key]

  list

_.clone = (list) ->
  return list unless _.isObject list

  if _.isArray(list)
    _.slice(list, 1, #list)
  else
    _.extend({}, list)

_.isEmpty = (value) ->
  if not value
    true
  elseif _.isArray(value) or _.isObject(value)
    next(value) == nil
  elseif _.isString(value)
    string.len(value) == 0
  else
    false

_.isNaN = (value) -> _.isNumber(value) and value ~= value
_.isObject = (value) -> type(value) == "table"
_.isArray = (value) -> type(value) == "table" and (value[1] or next(value) == nil)
_.isString = (value) -> type(value) == "string"
_.isNumber = (value) -> type(value) == "number"
_.isFunction = (value) -> type(value) == "function"
_.isBoolean = (value) -> type(value) == "boolean"
_.isNil = (value) -> value == nil
_.isFinite = (value) ->
  _.isNumber(value) and -math.huge < value and value < math.huge

_.tap = (value, func) ->
  func(value)
  value

splitIterator = (value, pattern, start) ->
  if pattern
    string.find(value, pattern, start)
  else
    if start > string.len(value)
      return nil
    else
      return start+1, start


_.split = (value, pattern) ->
  if not _.isString(value) then return {} end
  local values = {}
  local start = 1
  local start_pattern, end_pattern = splitIterator(value, pattern, start)

  while start_pattern
    table.insert values, string.sub value, start, start_pattern - 1
    start = end_pattern + 1
    start_pattern, end_pattern = splitIterator value, pattern, start

  if start <= string.len(value)
    table.insert(values, string.sub(value, start))

  values

_.chain = (value) -> _(value).chain()

id_counter = -1
_.uniqueId = (prefix) ->
  id_counter = id_counter + 1
  if prefix
    prefix .. id_counter
  else
    id_counter

_.times = (n, func) ->
  func(i) for i=0, (n-1), 1

result = (self, obj) ->
  if _.isObject(self) and self._chain then _(obj).chain() else obj


_.mixin = (obj) ->
  _.each _.functions(obj), (name) ->
    func = obj[name]
    _[name] = func

    chainable_mt[name] = (target, ...) ->
      r = func(target._wrapped, ...)
      if _.include {'pop','shift'}, name
        result target, target._wrapped
      else
        result target, r

entityMap =
  escape:
    '&': '&amp;'
    '<': '&lt;'
    '>': '&gt;'
    '"': '&quot;'
    "'": '&#x27;'
    '/': '&#x2F;'

entityMap.unescape = _.invert(entityMap.escape)

_.escape = (value='') ->
  value\gsub("[#{_(entityMap.escape).chain()\keys()\join()\value()}]", (s) ->
    entityMap['escape'][s]

_.unescape = (value='') ->
  _.each entityMap.unescape, (escaped, key) ->
    value = value\gsub(key, (s) -> escaped
  value

_.result = (obj, prop) ->
  return unless obj
  value = obj[prop]
  if _.isFunction(value) then value(obj) else value

_.print_r  = (t, name, indent) ->
  tableList = {}
  table_r = (t, name, indent, full) ->
    serial = string.len full == 0 and name or type(name) != "number" and '["'..tostring(name)..'"]' or "[#{name}]"
    io.write indent, serial, ' = '
    if type(t) == "table"
      if tableList[t] != nil then
        io.write '{}; -- ', tableList[t], ' (self reference)\n'
      else
        tableList[t] = full..serial
        if next t then -- Table not empty
          io.write '{\n'
          for key,value in pairs(t)
            table_r value, key, indent..'\t', full..serial
          io.write indent, '};\n'
        else
          io.write '{};\n'
    else
      io.write(type(t)~="number" and type(t)~="boolean" and '"'..tostring(t)..'"' or tostring(t),';\n')
  table_r(t,name or '__unnamed__',indent or '','')

_.collect = _.map
_.inject = _.reduce
_.foldl = _.reduce
_.foldr = _.reduceRight
_.detect = _.find
_.filter = _.select
_.every = _.all
_.same = _.any
_.contains = _.include
_.head = _.first
_.take = _.first
_.drop = _.rest
_.tail = _.rest
_.methods = _.functions

_.mixin(_)

setmetatable _,
  __call: (target, ...) ->
    wrapped = ...
    return wrapped if _.isObject(wrapped) and wrapped._wrapped

    instance = setmetatable({}, __index: chainable_mt)
    instance.chain = ->
      instance._chain = true
      instance
    instance.value = -> instance._wrapped

    instance._wrapped = wrapped
    instance

_
