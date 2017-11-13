local _ = require 'lodash'

local scriptPath = ''

--package.path = package.path .. ";../?.lua;" .. scriptPath .. '/?.lua'
package.path = package.path .. ";../?.lua;../../../scripts/lua/?.lua;" .. scriptPath .. '/?.lua;../device-adapters/?.lua;'

local HTTPResponse = require('HTTPResponse')

describe('HTTPResponse', function()

	it('should instantiate', function()

		local r = HTTPResponse({
			BASETYPE_HTTP_RESPONSE = 'httpResponse'
		}, {
			headers = {['Content-Type'] = 'application/json'},
			data  = '{"a":1}',
			callback = 'trigger1'
		})

		assert.is_same({a = 1}, r.json)
		assert.is_true(r.isJSON)
		assert.is_same('{"a":1}', r.data)
	end)

end)
