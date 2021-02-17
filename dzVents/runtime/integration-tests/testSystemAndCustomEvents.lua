local dumpfile =  '../../../scripts/dzVents/dumps/eventsIntegrationtests.triggers'

describe('Custom events', function()

	before_each(function()
	end)

	after_each(function() end)
	describe("Events", function()

		it('should log a customEvent triggered by api call', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "data: someencodedstring"  > /dev/null' ))
		end)

		it('should log a customEvent', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "trigger: myEvents3"  > /dev/null' ))
		end)

		it('should log a customEvent with string data', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "data: myEventString"  > /dev/null' ))
		end)

		it('should log a customEvent with table data', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "b: 2"  > /dev/null' ))
		end)

	end)

end)


describe('System events', function()

	before_each(function()
	end)

	after_each(function() end)
	describe("Events", function()

		it('should log a start event on the first line', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "type: start"  > /dev/null' ))
		end)

		it('should log a backup ', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "type: manualBackupFinished"  > /dev/null' ))
		end)

		it('should log a stop event ', function()
			assert.is_true( os.execute('cat ' .. dumpfile .. ' |  grep "type: stop"  > /dev/null' ))
		end)

		it('should log resetAllEvents ', function()
			assert.is_true(os.execute('cat ' .. dumpfile .. ' |  grep "type: resetAllEvents"  > /dev/null ' ))
		end)

		it('should log resetAllDeviceStatus ', function()
			assert.is_true(os.execute('cat ' .. dumpfile .. ' |  grep "type: resetAllDevice"   > /dev/null ' ))
		end)
	end)

end)

