local scriptPath = ''
 package.path = ";../?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;../../../scripts/lua/?.lua;' .. package.path

describe('lodash:', function()

	local _ = require 'lodash'
	local utils = require('Utils')

	setup(function()
		_G.logLevel = 1
		_G.log = function() end
		_G.globalvariables =
		{
			Security = 'sec',
			['radix_separator'] = '.',
			['script_path'] = scriptPath,
			['domoticz_listening_port'] = '8080'
		}
	end)

	pprint = function(...)
		return _.table(...)
	end

	showResult = function(...)
		local t = _.table(...)
		for i, v in ipairs(t) do
			t[i] = _.str(t[i])
		end
		return _.args(t)
	end

	teardown(function()
		utils = nil
		_ = nil
	end)

	describe("Section Array", function()

		 it('should have function _.chunk.', function()
			assert.is_true(_.isEqual(_.chunk({'x', 'y', 'z', 1, 2, 3, 4, true , false}, 4), {{"x", "y", "z", 1}, {2, 3, 4, true}, {false}}))
		 end)

		it('should have function _.compact.', function()
			assert.is_same(showResult(_.compact({'x', 'y', nil, 1, 2, 3, false, true , false})),'{"x", "y", 1, 2, 3, true}')
		 end)

		it('should have function _.difference.', function()
			assert.is_same(showResult(_.difference({3, 1, 2, 9, 5, 9}, {4, 5}, {9, 1})),'{3, 2}')
		end)

		it('should have function _.drop.', function()
			assert.is_same(showResult(_.drop({1, 2, 3, 4, 5, 6}, 2)),'{3, 4, 5, 6}')
		end)

		it('should have function _.dropRight.', function()
			assert.is_same(showResult(_.dropRight({1, 2, 3, 4, 5, 6}, 2)),'{1, 2, 3, 4}')
		end)

		it('should have function _.dropRightWhile.', function()
			assert.is_same(showResult(_.dropRightWhile({1, 5, 2, 3, 4, 5, 4, 4})),'{}')
		end)

		it('should have function _.dropWhile.', function()
			assert.is_same(showResult(_.dropWhile({1, 2, 2, 3, 4, 5, 4, 4, 2})),'{}')
		end)

		it('should have function _.enqueue.', function()
			t = {1, 2, 2, 3, 4, 5, 4, 4, 2}
			_.enqueue(t, 12)
			assert.is_same(showResult(t),'{12, 1, 2, 2, 3, 4, 5, 4, 4, 2}')
		end)

		it('should have function _.fill.', function()
			t = {1, 2, 3, 4}
			_.fill(t,'x', 2, 3)
			assert.is_same(showResult(t),'{1, "x", "x", 4}')
		end)

		it('should have function _.findIndex.', function()
			assert.is_same(showResult(_.findIndex({{a = 1}, {a = 2}, {a = 3}, {a = 2}, {a = 3}})),'1')
		end)

		it('should have function _.findLastIndex.', function()
			assert.is_same(showResult(_.findLastIndex({{a = 1}, {a = 2}, {a = 3}, {a = 2}, {a = 3}})),'5')
		end)

		it('should have function _.first.', function()
			assert.is_same(showResult(_.first({'w', 'x', 'y', 'z'})),'w')
		end)

		it('should have function _.flatten.', function()
			assert.is_same(showResult(_.flatten({1, 2, {3, 4, {5, 6}}})),'{1, 2, 3, 4, {5, 6}}')
		end)

		it('should have function _.flattenDeep.', function()
			assert.is_same(showResult(_.flattenDeep({1, 2, {3, 4, {5, 6}}})),'{1, 2, 3, 4, 5, 6}')
		end)

		it('should have function _.indexOf.', function()
			assert.is_same(showResult(_.indexOf({2, 3, 'x', 4}, 'x')),'3')
		end)

		it('should have function _.initial.', function()
			assert.is_same(showResult(_.initial({1, 2, 3, 'a'})),'{1, 2, 3}')
		end)

		it('should have function _.intersection.', function()
			assert.is_same(showResult(_.intersection({1, 2}, {4, 2}, {2, 1})),'{2}')
		end)

		it('should have function _.last.', function()
			assert.is_same(showResult(_.last({'w', 'x', 'y', 'z'})),'z')
		end)

		it('should have function _.lastIndexOf.', function()
			assert.is_same(showResult(_.lastIndexOf({2, 3, 'x', 4, 'x', 5}, 'x')),'5')
		end)

		it('should have function _.pull.', function()
			local t = {1, 2, 3, 4, 5, 4, 1, 2, 3}
			_.pull(t, 2, 3)
			assert.is_same(showResult(t,'{1, 4, 5, 4, 1}'))
		end)

		it('should have function _.pullAt.', function()
			local t = {1, 2, 4}
			_.pullAt(t, 3)
			assert.is_same(showResult(t,'{1, 2}'))
		end)

		it('should have function _.push.', function()
			local t = {1, 2, 3}
			_.push(t, 4)
			assert.is_same(showResult(t,'{1, 2, 3, 4}'))
		end)

		it('should have function _.remove.', function()
			local t = {1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 1, 2, 3, 5, 4}
			local removed = _.remove(t, function(value)
				return value > 4
			end)
			assert.is_same(showResult(t, '{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4}' ))
			assert.is_same(showResult(removed, '{5, 6, 7, 5, 6, 5}'))
		end)

		it('should have function _.rest.', function()
			assert.is_same(showResult(_.rest({1, 2, 3, 'a'})),'{2, 3, "a"}')
		end)

		it('should have function _.reverse.', function()
			assert.is_same(showResult(_.reverse({1, 2, 3, 'a', 'b'})),'{"b", "a", 3, 2, 1}')
		end)

		it('should have function _.slice.', function()
			assert.is_same(showResult(_.slice({1, 2, 3, 4, 5, 6}, 2, 3)),'{2, 3}')
		end)

		it('should have function _.take.', function()
			assert.is_same(showResult(_.take({1, 2, 3, 4, 5}, 3)),'{1, 2, 3}')
		end)

		it('should have function _.takeRight.', function()
			assert.is_same(showResult(_.takeRight({1, 2, 3, 4, 5}, 3)),'{3, 4, 5}')
		end)

		it('should have function _.takeRightWhile.', function()
			assert.is_same(showResult(_.takeRightWhile({1, 2, 3, 4, 5, 6, 7, 8})),'{1, 2, 3, 4, 5, 6, 7, 8}')
		end)

		it('should have function _.takeWhile.', function()
			assert.is_same(showResult(_.takeWhile({1, 2, 3, 4, 5, 6, 7, 8})),'{1, 2, 3, 4, 5, 6, 7, 8}')
		end)

		it('should have function _.union.', function()
			assert.is_same(showResult(_.union({1, 2}, {4, 2}, {2, 1})),'{1, 2, 4}')
		end)

		it('should have function _.uniq.', function()
			assert.is_same(showResult(_.uniq({1, 3, 2, 2})),'{1, 3, 2}')
		end)

		it('should have function _.unzip.', function()
			local t = _.zip({'a', 'b', 'c'}, {1, 2, 3}, {10, 20, 30})
			assert.is_same(showResult(_.unzip(t), '{{"a", "b", "c"}, {1, 2, 3}, {10, 20, 30}}'))
		end)

		it('should have function _.without.', function()
			assert.is_same(showResult(_.without({1, 1, 2, 3, 2, 3, 5, 5, 1, 2}, 5, 1)),'{2, 3, 2, 3, 2}' )
		end)

		it('should have function _.zip.', function()
			local t = _.zip({'a', 'b', 'c'}, {1, 2, 3}, {10, 20, 30})
			assert.is_same(showResult(t), '{{"a", 1, 10}, {"b", 2, 20}, {"c", 3, 30}}')
		end)

		it('should have function _.zipObject.', function()
			local t = {{'fred', 30}, {'alex', 20}}
			local t = _.zipObject(t)
			assert.is_same(showResult(t.fred),'30')
			assert.is_same(showResult(t.alex),'20')
		end)
	end)

	describe("Section Collection", function()

		it('should have function _.at', function()
			assert.is_same(showResult(_.at({'1', '2', '3', '4', a='a', b='b'}, {1, 2}, 'b')), '{"1", "2", "b"}' )
		end)

		it('should have function _.countBy.', function()
			assert.is_same(showResult(_.countBy({'dzVents' , 'dzVents', 4.3, 6.1, 6.4, 6.1}).dzVents,'2'))
		end)

		it('should have function _.indexBy.', function()
			local keyData = { {dir='l', a=1}, {dir='r', a=2}}
			local t = _.indexBy(keyData, function(n)
				return n.dir
			end)
			assert.is_same(showResult(t.r.a,'2'))
		end)

		it('should have function _.every.', function()
			_.every({1, 2, 3, 4, '5', 6}, _.isNumber)
			assert.is_false(_.every({1, 2, 3, 4, '5', 6}, _.isNumber))
			assert.is_true(_.every({1, 2, 3, 4, 5, 6.2}, _.isNumber))
		end)

		it('should have function _.filter.', function()
			assert.is_same(showResult(_.filter({1, 2, 3, 4, '5', 6, '7'}, _.isNumber),'{1, 2, 3, 4, 6}'))
		end)

		it('should have function _.find.', function()
			local t = {{a = 1}, {a = 2}, {a = 3}, {a = 2}, {a = 3}}
			assert.is_true(_.isEqual(_.find(t, function(v) return v.a == 3 end ), { a = 3 }))
		end)

		it('should have function _.findLast.', function()
			local t = {{a = 1}, {a = 2}, {a = 3, x = 1}, {a = 2}, {a = 3, x = 2}}
			assert.is_true(_.isEqual(_.findLast(t, function(v) return v.a == 3 end ), {x = 2, a = 3}))
		end)

		it('should have function _.forEach.', function()
			assert.is_same(showResult(_.forEach({1, 2, 3, 4, '5', 6, '7'}),'{1, 2, 3, 4, "5", 6, "7"}'))
		end)

		it('should have function _.forEachRight.', function()
			assert.is_same(showResult(_.forEachRight({1, 2, 3, 4, '5', 6, '7'}),'{1, 2, 3, 4, "5", 6, "7"}'))
		end)

		it('should have function _.groupBy.', function()
			local t = _.groupBy({4.3, 6.1, 6.4}, function(n)
				return math.floor(n)
			end)
			assert.is_true(_.isEqual(t, {["4"]={4.3}, ["6"]={6.1, 6.4}}))
		end)

		it('should have function _.includes.', function()
			assert.is_true(_.includes({1, 2, 'x', 3, ['5']=4, x=3, 5}, 'x'))
			assert.is_false(_.includes({1, 2, 'x', 3, ['5']=4, x=3, 5}, 'z'))
		end)

		it('should have function _.invoke.', function()
			assert.is_same(showResult(_.invoke({'1.first', '2.second', '3.third'}, string.sub, 1, 1),'{"1", "2", "3"}'))
		end)

		it('should have function _.map.', function()
			local t = _.map({1, 2, 3, 4, 5, 6, 7, 8, 9}, function(n)
				return n * n
			end)
			assert.is_same(showResult(t,'{1, 4, 9, 16, 25, 36, 49, 64, 81}'))
		end)

		it('should have function _.partition.', function()
			local t = _.partition({1, 2, 3, 4, 5, 6, 7}, function (n)
				return n > 3
			end)
			assert.is_same(showResult(t,'{{4, 5, 6, 7}, {1, 2, 3}}'))
		end)

		it('should have function _.pluck.', function()
			local users = {{ user = 'barney', age = 36, child = {age = 5}}, { user = 'fred', age = 40, child = {age = 6} }}
			assert.is_same(showResult(_.pluck(users, {'user'})),'{"barney", "fred"}')
			assert.is_same(showResult(_.pluck(users, {'user'})),'{"barney", "fred"}')
		end)

		it('should have function _.reduce.', function()
			 assert.is_same(showResult(_.reduce({1, 2, 3}, function(total, n) return n + total end), '6'))
			 local expectedResult = { b = 6, a = 3}
			 assert.is_true(_.isEqual(_.reduce({a = 1, b = 2},
				function(result, n, key)
					result[key] = n * 3
					return result
				end, {}), expectedResult ))
		end)

		it('should have function _.reduceRight.', function()
			assert.is_same(showResult(_.reduceRight({0, 1, 2, 3, 4, 5},
				function(str, other)
					return str .. other
				end ), '543210'))
		end)

		it('should have function _.reject.', function()
			assert.is_same(showResult(_.reject({1, 2, 3, 4, '5', 6, '7'}, _.isNumber),'{"5", "7"}'))
		end)

		it('should have function _.sample', function()
			local t = {1, 2, 3, a=4, b='x', 5, 23, 24}
			local sample = _.sample(t, 98)
			assert.is_same(#sample, 98)
			local sample = _.sample(t)
			assert.is_true(_.includes(t, sample))
		end)

		it('should have function _.size.', function()
			assert.is_equal(_.size({'abc', 'def'}), 2)
			assert.is_equal(_.size('abcdefg'), 7)
			assert.is_equal(_.size({a=1, b=2, c=3}), 3)
		end)

		it('should have function _.some.', function()
			assert.is_false(_.some({1, 2, 3, 4, 5, 6}, _.isString))
			assert.is_true(_.some({1, 2, '3', 4, 5, 6}, _.isString))
		end)

		it('should have function _.sortBy.', function()
			local t = {1, 2, 3}
			assert.is_true(_.isEqual(_.sortBy(t, function (a) return math.sin(a) end), {1, 3, 2}))
			local users = { { user='fred'} , { user='alex' }, { user='zoee' }, { user='john' }}
			assert.is_same(_.sortBy(users, function (a)
												return a.user
											end), { { user='alex'}, { user='fred' }, { user='john' }, { user='zoee' }})
		end)
	end)

	describe("Section Function", function()

		it('should have function _.after', function()
			local out = {}
			local function pprint(str)
				out[#out+1] = str
			end

			local printAfter3 = _.after(3, pprint)
			for i = 1, 5 do
				printAfter3('done ' .. i)
			end

			assert.is_same(showResult( out , '{"done 4", "done 5"}'))
			assert.is_same(out , { [1] = "done 4" , [2] = "done 5" })
		end)

		it('should have function _.ary', function()
			local printOnly3 = _.ary(pprint, 3)
			assert.is_same( _.slice(printOnly3('a', 3, 'x', 'y', 6), 1, 3) , {'a', 3, 'x' })
		end)

		it('should have function _.before', function()
			local out = {}

			local function pprint(...)
				local t = _.table(...)
				for i, v in ipairs(t) do
					out[#out + 1] = _.str(t[i])
				end
			end

			local printBefore3 = _.before(3, pprint)
			for i = 1, 10 do printBefore3(i .. ' OK') end
			assert.is_same(out, { [1] = '1 OK', [2] = '2 OK', [3] = '3 OK' })
		end)

		it('should have function _.modArgs', function()
			local out = {}

			local function pprint(...)
				local t = _.table(...)
				for i, v in ipairs(t) do
					out[#out + 1] = _.str(t[i])
				end
			end

			local increment = function(...)
				return _.args(_.map(_.table(...), function(n)
					return n + 1
				end))
			end

			local pow = function(...)
				return _.args(_.map(_.table(...), function(n)
					return n * n
				end))
			end

			local modded = _.modArgs(function(...)
				pprint(...)
			end, {pow, increment, increment}, { pow, increment }, increment, increment)

			modded(0, 1, 2)

			assert.is_same(out, { [1] = '7', [2] = '12', [3] = '39' })
		end)

		it('should have function _.negate', function()
			local isEven = function (n)
				return n % 2 == 0
			end
			local isOdd = _.negate(isEven)

			assert.is_same(_.filter({1, 2, 3, 4, 5, 6}, isEven), {2, 4, 6 })
			assert.is_same(_.filter({1, 2, 3, 4, 5, 6}, isOdd), {1, 3, 5})
		end)

		it('should have function _.once', function()
			local out = {}

			local function pprint(...)
				local t = _.table(...)
				for i, v in ipairs(t) do
					out[#out + 1] = _.str(t[i])
				 end
			end

			local createApp = function(version)
				pprint('App created with version '.. version)
				return version
			end

			local initialize = _.once(createApp)
			initialize(1.1)
			out[#out+1] = initialize(1.1)

			assert.is_same(out[1], 'App created with version 1.1' )
			assert.is_same(out[2], 1.1 )

		end)

		it('should have function _.rearg', function()
			local rearged = _.rearg(function(a, b, c)
				return {a, b, c};
			end, 2, 1, 3)

			assert.is_same(showResult(rearged('a', 'b', 'c'), '{"b", "a", "c"}' ))
		end)

	end)

	describe("Section Language specific", function()
		it('should have function _.args', function()
			assert.is_same({_.args({1, 2, 3, 'a', })}, {[1] = 1, [2] = 2, [3] = 3 , [4] = 'a'})
		end)

		it('should have function _.bool', function()
			assert.is_false(_.bool({1, 2, 3}))
			assert.is_true(_.bool("123"))
			assert.is_false(_.bool(0))
			assert.is_false(_.bool(nil))
			assert.is_true(_.bool(function(a) return a end, "555" ))
		end)

		it('should have function _.func', function()
			local f = _.func(1, 2, 3)
			assert.is_same({ f() }, {[1] = 1, [2] = 2, [3] = 3})
		end)

		it('should have function _.gt', function()
			assert.is_false(_.gt(1, 3))
			assert.is_false(_.gt(3, 3))
			assert.is_true(_.gt(3.1, 3))
		end)

		it('should have function _.gte', function()
			assert.is_false(_.gte(1, 3))
			assert.is_true(_.gte(3, 3))
			assert.is_true(_.gte(3.1, 3))
		end)

		it('should have function _.isBoolean', function()
			assert.is_true(_.isBoolean(false))
			assert.is_true(_.isBoolean(true))
			assert.is_false(_.isBoolean({}))
			assert.is_false(_.isBoolean(a))
			assert.is_false(_.isBoolean('a'))
			assert.is_false(_.isBoolean(math.pi))
		end)

		it('should have function _.isEmpty', function()
			assert.is_true(_.isEmpty(true))
			assert.is_true(_.isEmpty(1))
			assert.is_false(_.isEmpty({1}))
			assert.is_false(_.isEmpty({1, 2, 3}))
			assert.is_false(_.isEmpty({a = 1 }))
		end)

		it('should have function _.isFunction', function()
			assert.is_true(_.isFunction(function() end))
			assert.is_false(_.isFunction(1))
		end)

		it('should have function _.isNil', function()
			assert.is_true(_.isNil(var))
			local var = 1
			assert.is_false(_.isNil(var))
		end)

		it('should have function _.isNumber', function()
			assert.is_true(_.isNumber(1))
			assert.is_true(_.isNumber(1.12e3))
			assert.is_true(_.isNumber(1.1234))
			assert.is_false(_.isNumber('1'))
		end)

	 it('should have function _.isString', function()
			assert.is_true(_.isString( '1' ))
			assert.is_false(_.isString( 1 ))
			assert.is_false(_.isString( {1} ))
			assert.is_false(_.isString( 1.12e4 ))
			assert.is_false(_.isString( true ))
			assert.is_false(_.isString( function() end ))
		end)

		it('should have function _.isTable', function()
			assert.is_true(_.isTable( {} ))
			assert.is_true(_.isTable( {1} ))
			assert.is_false(_.isTable( 'a' ))
			assert.is_false(_.isTable( 1 ))
			assert.is_false(_.isTable( true ))
			assert.is_false(_.isTable( function() end ))
		end)

		it('should have function _.lt', function()
			assert.is_true(_.lt(1, 3))
			assert.is_false(_.lt(3, 3))
			assert.is_false(_.lt(3.1, 3))
		end)

		it('should have function _.lte', function()
			assert.is_true(_.lte(1, 3))
			assert.is_true(_.lte(3, 3))
			assert.is_false(_.lte(3.1, 3))
		end)

		it('should have function _.num', function()
			assert.is_same(_.num({1, 2, 3}), 0)
			assert.is_same(_.num("123"), 123)
			assert.is_same(_.num(true), 1)
			assert.is_same(_.num(function(a) return a end, "555"), 555)
		end)

		it('should have function _.str', function()
			assert.is_same(_.str({1, 2, 3, 4, {k=2, {'x', 'y'}}}),'{1, 2, 3, 4, {{"x", "y"}, ["k"]=2}}')
			assert.is_same(_.str({1, 2, 3, 4, function(a) return a end}, 5),'{1, 2, 3, 4, 5}')
		end)

		it('should have function _.table', function()
			assert.is_true(_.isEqual(_.slice(_.table(1, 2, 3), 1, 3) , {[1] = 1, [2] = 2, [3] = 3}))
			assert.is_true(_.isEqual(_.slice(_.table("123"), 1, 1), { [1] = "123"}))
		end)
	end)

	describe("Section Number", function()

	 it('should have function _.inRange', function()
			assert.is_true(_.inRange(-3, -4, 8))
			assert.is_true(_.inRange(-3.5, -4.32, 8))
			assert.is_true(_.inRange(-3, -3, 8))
			assert.is_false(_.inRange(-4.5, -3, 8))
		end)

		it('should have function _.random', function()
		 assert.is_true(_.inRange(_.random(), 0, 1.1))
		 assert.is_true(_.inRange(_.random(5), 0, 5.1))
		 assert.is_true(_.inRange(_.random(4, 5, true), 4, 6))
		end)

	 it('should have function _.round', function()
			assert.is_same(_.round(math.pi), 3 )
			assert.is_same(_.round(math.pi, 2), 3.14 )
			assert.is_same(_.round(math.pi, 16), 3.1415926535897931170 )
		end)
	end)

	describe("Section Object", function()
		it('should have function _.findKey', function()
			local key = _.findKey({a={a = 1}, b={a = 2}, c={a = 3, x = 1}}, function(v) return v.a == 3 end)
			assert.is_same(key,'c')
		end)
		it('should have function _.findLastKey', function()
			local lastKey = _.findLastKey({a={a=3}, b={a = 2}, c={a=3, x = 1}}, function(v) return v.a == 2 end)
			assert.is_same(lastKey,'b')
		end)

		it('should have function _.functions', function()
			assert.is_same(_.functions(debug),	{	"debug", "gethook", "getinfo", "getlocal", "getmetatable", "getregistry",
													"getupvalue", "getuservalue", "sethook", "setlocal", "setmetatable",
													"setupvalue", "setuservalue", "traceback", "upvalueid", "upvaluejoin"} )
			assert.is_same(_.functions(string), {	"byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower",
													"match", "pack", "packsize", "rep", "reverse","sMatch", "sub", "unpack", "upper"})
			assert.is_same(_.functions(math),	{	"abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "deg", "exp",
													"floor", "fmod", "frexp", "ldexp", "log", "log10", "max", "min", "modf",
													"pow", "rad", "random", "randomseed", "sin", "sinh", "sqrt", "tan", "tanh",
													"tointeger", "type", "ult"})
			assert.is_same(_.functions(io),		{	"close", "flush", "input", "lines", "open", "output", "popen", "read",
													"tmpfile", "type", "write"})
			assert.is_same(_.functions(os),		{	"clock", "date", "difftime", "execute", "exit", "getenv", "remove", "rename",
													"setlocale", "time", "tmpname"})
			assert.is_same(_.functions(package),{	"loadlib", "searchpath"})
			assert.is_same(_.functions(table),	{	"concat", "insert", "move", "pack", "remove", "sort", "unpack"})
			assert.is_same(_.functions(utf8),	{	"char", "codepoint", "codes", "len", "offset"})
		end)

		it('should have function _.get', function()
			local object = {a={b={c={d=5}}}}
			assert.is_same(_.get(object, {'a', 'b', 'c'}), {["d"] = 5})
		end)

		it('should have function _.has', function()
			local object = {a={b={c={d}}}}
			assert.is_true(_.has(object, {'a', 'b', 'c'}))
			assert.is_false(_.has(object, {'a', 'b', 'd'}))
		end)

		it('should have function _.invert', function()
			assert.is_same(_.invert({a='1', b='2', c='3', d='3'}), {["3"]="d", ["2"]="b", ["1"]="a"} )
			assert.is_same(_.invert({a= 1, b= 2 , c = 3, d = 4 }), {[3] = "c", [2] = "b", [1] = "a", [4] = "d"} )
		end)

		it('should have function _.keys', function()
			assert.is_same(_.keys("test"), {1, 2, 3, 4})
			assert.is_same(_.keys({ a = 1, b = 2, c = 3} ), {'a', 'b', 'c'})
		end)

		it('should have function _.pairs', function()
			assert.is_same(_.pairs({1, 2, 'x', a='b'}), {{1, 1}, {2, 2}, {3, 'x'}, {'a', 'b'}})
		end)

		it('should have function _.result', function()
			local object = {a=5, b={c=function(a) return(a) end}}
			assert.is_same(_.result(object, {'b', 'c'}, nil, 5), 5)

		end)

		it('should have function _.values', function()
			assert.is_same(_.values("test"), {'t','e','s','t'})
			assert.is_same(_.values({a=1, b=3, c=2}), {1, 3, 2})
			assert.is_same(_.values({a= { b=3, c=2}}), {{ b = 3 , c = 2}})
		end)
	end)

	describe("Section Utility", function()

		it('should have function _.constant', function()
			local object = {x=5}
			local getter = _.constant(object)
			assert.is_true(getter() == object)
		end)

		it('should have function _.identitiy', function()
			local object = {x=5}
			assert.is_true(_.identity(object) == object)
		end)

		it('should have function _.noop', function()
			local object = {x=5}
			assert.is_nil(_.noop(object))
		end)

		it('should have function _.range', function()
		
			assert.is_same(_.range(4, 36, 4),{4, 8, 12, 16, 20, 24, 28, 32, 36})
		end)
	end)


	describe("Section Should come last", function()
		it('should have function _.print', function()
			_.print('')
		end)
	end)
end)
