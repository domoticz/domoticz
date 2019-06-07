return {
	on = {
		devices = {'vdDocumentationSwitch'},
	},
	
	execute = function(dz, item)
		local documents = {'history.md', 'README.md', 'README.wiki' }
		local documentPath = globalvariables.runtime_path:gsub('runtime','documentation')
		local docVar = dz.variables('varUpdateDocument')
		local updateDocfails = 0
		
		local function osExecute(cmd)
			local fileHandle = assert(io.popen(cmd, 'r'))
			local commandOutput = assert(fileHandle:read('*a'))
			local returnTable = {fileHandle:close()}
			
			local log = dz.LOG_DEBUG
			local msg = "0 (OK)"
			if returnTable[3] ~= 0 then
				log = dz.LOG_ERROR
				if commandOutput == nil or commandOutput == '' then
					commandOutput = '- failed -'
				end
				msg = returnTable[3] .. " ( " .. commandOutput .." )"
			end
			dz.log("Command: " .. cmd .. ", returnCode: " .. msg, log )
			return returnTable[3] -- rc[3] is returnCode
		end
	
		local function grep (file, str, isLinux )
			return ( isLinux and  osExecute("grep " .. str .." " .. file ) ) or ( os.execute("findstr " .. str .. " " .. file ) )
		end
		
		docVar.set(99) -- if something unexpected happens we will notice it because of this setting
		for _,document in ipairs(documents) do	
			if not dz.utils.fileExists(documentPath .. document) then
				print ('shit ' .. documentPath .. document .." does not exist")
			else
				print (documentPath .. document .." found")
				local rc = grep( documentPath .. document, globalvariables.dzVents_version, osExecute("sleep 0" ) ) 
				if rc ~= 0  then
					dz.log("Problems !! ( Returncode: " .. rc .. ") ", dz.LOG_ERROR )
					updateDocfails = updateDocfails + 1
				end
			end
		end
		docVar.set(updateDocfails) 
		
	end
}
