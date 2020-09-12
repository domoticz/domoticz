local scriptPath = ''

package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'
package.path = package.path .. ";../?.lua;../../../scripts/lua/?.lua;" .. scriptPath .. '/?.lua;'

local HTTPResponse = require('HTTPResponse')

describe('HTTPResponse', function()

	it('should instantiate', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['Content-Type'] = 'application/json'},
			data  = '{"a":1}',
			callback = 'trigger1',
			statusCode = 404,
			statusText = "empty",
			protocol = "HTTP/1.1",
		},"testHTTPResponse")

		assert.is_same({a = 1}, r.json)
		assert.is_true(r.isJSON)
		assert.is_same('{"a":1}', r.data)

		assert.is_true(r.isHTTPResponse)
		assert.is_false(r.isVariable)
		assert.is_false(r.isTimer)
		assert.is_false(r.isScene)
		assert.is_false(r.isDevice)
		assert.is_false(r.isGroup)
		assert.is_false(r.isSecurity)
		assert.is_same(404, r.statusCode)
		assert.is_false(r.ok)
	end)

	it('should have a valid statuscode based on statusCode', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['Content-Type'] = 'application/json'},
			data  = '{"a":1}',
			callback = 'trigger1',
			statusCode = 200
		})

		assert.is_true(r.ok)
	end)

	it('should have a valid statuscode', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['content-type'] = 'application/json' },
			statusText = 'Empty response' ,
			protocol = 'HTTP/1.4' ,
			statusCode  = 404 ,
			data  = '{"a":1}',
			callback = 'trigger1',

		},"testHTTPResponse")
		assert.is_same('HTTP/1.4', r.protocol)
		assert.is_same(404, r.statusCode)
		assert.is_false(r.ok)
		assert.is.same('Empty response',r.statusText)
		assert.is_true(r.isJSON)
		assert.is_same({a=1},r.json)

	end)

		it('should have a valid statustext', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['Content-Type'] = 'application/json' },
			data  = '{"a":1}',
			callback = 'trigger1',
			statusText = '' ,
			protocol = '' ,
			statusCode = 0 ,

		},"testHTTPResponse")
		assert.is_same('', r.protocol)
		assert.is_same(0,r.statusCode)
		assert.is_false(r.ok)
		assert.is_same('',r.statusText)
	end)

	it('should return a valid JSON table ', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['content-type'] = 'application/json' },
			statusText = 'OK' ,
			protocol = 'HTTP/2.2' ,
			statusCode  = 200 ,
			data  = '{"a":1}',
			callback = 'trigger1',

		},"testHTTPResponse")
		assert.is_same('HTTP/2.2', r.protocol)
		assert.is_same(200, r.statusCode)
		assert.is_true(r.ok)
		assert.is.same('OK',r.statusText)
		assert.is_true(r.isJSON)
		assert.is_same({a=1},r.json)

	end)

	it('should recognize other content (~= JSON)', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['content-type'] = 'application/xml' },
			statusText = 'OK' ,
			protocol = 'HTTP/1.4' ,
			statusCode  = 200 ,
			data  = "<note><to>Tove</to><from>Jani</from><heading>Reminder</heading><body>Don't forget me this weekend!</body></note>",
			callback = 'trigger1',

		},"testHTTPResponse")
		assert.is_same('HTTP/1.4', r.protocol)
		assert.is_same(200, r.statusCode)
		assert.is_true(r.ok)
		assert.is.same('OK',r.statusText)
		assert.is_false(r.isJSON)
		assert.is_nil(r.json)
		assert.is_same('application/xml',r._contentType)

	end)

end)
