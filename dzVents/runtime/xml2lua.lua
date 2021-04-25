--- @module Module providing a non-validating XML stream parser in Lua. 
--  
--  Features:
--  =========
--  
--		* Tokenises well-formed XML (relatively robustly)
--		* Flexible handler based event API (see below)
--		* Parses all XML Infoset elements - ie.
--			- Tags
--			- Text
--			- Comments
--			- CDATA
--			- XML Decl
--			- Processing Instructions
--			- DOCTYPE declarations
--		* Provides limited well-formedness checking 
--		(checks for basic syntax & balanced tags only)
--		* Flexible whitespace handling (selectable)
--		* Entity Handling (selectable)
--  
--  Limitations:
--  ============
--  
--		* Non-validating
--		* No charset handling 
--		* No namespace support 
--		* Shallow well-formedness checking only (fails
--		to detect most semantic errors)
--  
--  API:
--  ====
--
--  The parser provides a partially object-oriented API with 
--  functionality split into tokeniser and handler components.
--  
--  The handler instance is passed to the tokeniser and receives
--  callbacks for each XML element processed (if a suitable handler
--  function is defined). The API is conceptually similar to the 
--  SAX API but implemented differently.
--
--  XML data is passed to the parser instance through the 'parse'
--  method (Note: must be passed a single string currently)
--
--  License:
--  ========
--
--		This code is freely distributable under the terms of the [MIT license](LICENSE).
--
--
--@author Paul Chakravarti (paulc@passtheaardvark.com)
--@author Manoel Campos da Silva Filho
local xml2lua = {}
local XmlParser = require("XmlParser")

---Recursivelly prints a table in an easy-to-ready format
--@param tb The table to be printed
--@param level the indentation level to start with
local function printableInternal(tb, level)
  if tb == nil then
	 return
  end
  
  level = level or 1
  local spaces = string.rep(' ', level*2)
  for k,v in pairs(tb) do
		if type(v) == "table" then
		 print(spaces .. k)
		 printableInternal(v, level+1)
		else
		 print(spaces .. k..'='..v)
		end
  end  
end

---Instantiates a XmlParser object to parse a XML string
--@param handler Handler module to be used to convert the XML string
--to another formats. See the available handlers at the handler directory.
-- Usually you get an instance to a handler module using, for instance:
-- local handler = require("xmlhandler/tree").
--@return a XmlParser object used to parse the XML
--@see XmlParser
function xml2lua.parser(handler)	
	if handler == xml2lua then
		error("You must call xml2lua.parse(handler) instead of xml2lua:parse(handler)")
	end

	local options = { 
			--Indicates if whitespaces should be striped or not
			stripWS = 1, 
			expandEntities = 1,
			errorHandler = function(errMsg, pos) 
				error(string.format("%s [char=%d]\n", errMsg or "Parse Error", pos))
			end
			}

	return XmlParser.new(handler, options)
end

---Recursivelly prints a table in an easy-to-ready format
--@param tb The table to be printed
function xml2lua.printable(tb)
	printableInternal(tb)
end

---Handler to generate a string prepresentation of a table
--Convenience function for printHandler (Does not support recursive tables).
--@param t Table to be parsed
--@return a string representation of the table
function xml2lua.toString(t)
	local sep = ''
	local res = ''
	if type(t) ~= 'table' then
		return t
	end

	for k,v in pairs(t) do
		if type(v) == 'table' then 
			v = xml2lua.toString(v)
		end
		res = res .. sep .. string.format("%s=%s", k, v)	
		sep = ','
	end
	res = '{'..res..'}'

	return res
end

--- Loads an XML file from a specified path
-- @param xmlFilePath the path for the XML file to load
-- @return the XML loaded file content
function xml2lua.loadFile(xmlFilePath)
	local f, e = io.open(xmlFilePath, "r")
	if f then
		--Gets the entire file content and stores into a string
		return f:read("*a")
	end
	
	error(e)
end

---Gets an _attr element from a table that represents the attributes of an XML tag,
--and generates a XML String representing the attibutes to be inserted
--into the openning tag of the XML
--
--@param attrTable table from where the _attr field will be got
--@return a XML String representation of the tag attributes
local function attrToXml(attrTable)
  local s = ""
  local attrTable = attrTable or {}
  
  for k, v in pairs(attrTable) do
		s = s .. " " .. k .. "=" .. '"' .. v .. '"'
  end
  return s
end

---Gets the first key of a given table
local function getFirstKey(tb)
   if type(tb) == "table" then
		for k, v in pairs(tb) do
			return k
		end
		return nil
   end

   return tb
end

---Converts a Lua table to a XML String representation.
--@param tb Table to be converted to XML
--@param tableName Name of the table variable given to this function,
--				 to be used as the root tag.
--@param level Only used internally, when the function is called recursively to print indentation
--
--@return a String representing the table content in XML
function xml2lua.toXml(tb, tableName, level)
  local level = level or 0
  local firstLevel = level
  local spaces = string.rep(' ', level*2)
  local xmltb = level == 0 and {'<'..tableName..'>'} or {}

  for k, v in pairs(tb) do
		if type(v) == "table" then
		 --If the keys of the table are a number, it represents an array
		 if type(k) == "number" then
			local attrs = attrToXml(v._attr)
			v._attr = nil
			table.insert(xmltb, 
				spaces..'<'..tableName..attrs..'>\n'..xml2lua.toXml(v, tableName, level+1)..
				'\n'..spaces..'</'..tableName..'>') 
		 else 
			level = level + 1
			if type(getFirstKey(v)) == "number" then 
				table.insert(xmltb, spaces..xml2lua.toXml(v, k, level))
			else
				table.insert(
				 xmltb, 
				 spaces..'<'..k..'>\n'.. xml2lua.toXml(v, level+1)..
				 '\n'..spaces..'</'..k..'>')
			end
		 end
		else
		 table.insert(xmltb, spaces..'<'..k..'>'..tostring(v)..'</'..k..'>')
		end
  end

  if firstLevel == 0 then
	 table.insert(xmltb, '</'..tableName..'>\n')
  end
  return table.concat(xmltb, "\n")
end

return xml2lua
