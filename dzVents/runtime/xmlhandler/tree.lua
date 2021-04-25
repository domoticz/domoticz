local function init()
	local obj = {
		root = {},
		options = {noreduce = {}}
	}

	obj._stack = {obj.root, n=1}  
	return obj  
end

--- @module XML Tree Handler.
-- Generates a lua table from an XML content string.
-- It is a simplified handler which attempts
-- to generate a more 'natural' table based structure which
-- supports many common XML formats.
--
-- The XML tree structure is mapped directly into a recursive
-- table structure with node names as keys and child elements
-- as either a table of values or directly as a string value
-- for text. Where there is only a single child element this
-- is inserted as a named key - if there are multiple
-- elements these are inserted as a vector (in some cases it
-- may be preferable to always insert elements as a vector
-- which can be specified on a per element basis in the
-- options).  Attributes are inserted as a child element with
-- a key of '_attr'.
--
-- Only Tag/Text & CDATA elements are processed - all others
-- are ignored.
--
-- This format has some limitations - primarily
-- 
-- * Mixed-Content behaves unpredictably - the relationship
--   between text elements and embedded tags is lost and
--   multiple levels of mixed content does not work
-- * If a leaf element has both a text element and attributes
--   then the text must be accessed through a vector (to
--   provide a container for the attribute)
--
-- In general however this format is relatively useful.
--
-- It is much easier to understand by running some test
-- data through 'testxml.lua -simpletree' than to read this)
--
-- Options
-- =======
--	options.noreduce = { <tag> = bool,.. }
--		- Nodes not to reduce children vector even if only
--		  one child
--
--  License:
--  ========
--
--  This code is freely distributable under the terms of the [MIT license](LICENSE).
--
--@author Paul Chakravarti (paulc@passtheaardvark.com)
--@author Manoel Campos da Silva Filho
local tree = init()

---Instantiates a new handler object.
--Each instance can handle a single XML.
--By using such a constructor, you can parse
--multiple XML files in the same application.
--@return the handler instance
function tree:new()
	local obj = init()

	obj.__index = self
	setmetatable(obj, self)

	return obj
end

--Gets the first key of a table
--@param tb table to get its first key
--@return the table's first key, nil if the table is empty
--or the given parameter if it isn't a table
local function getFirstKey(tb)
   if type(tb) == "table" then
	  for k, v in pairs(tb) do
		  return k
	  end

	  return nil
   end

   return tb
end

--- Recursively removes redundant vectors for nodes
-- with single child elements
function tree:reduce(node, key, parent)
	for k,v in pairs(node) do
		if type(v) == 'table' then
			self:reduce(v,k,node)
		end
	end
	if #node == 1 and not self.options.noreduce[key] and 
		node._attr == nil then
		parent[key] = node[1]
	else
		node.n = nil
	end
end

---Parses a start tag.
-- @param tag a {name, attrs} table
-- where name is the name of the tag and attrs
-- is a table containing the atributtes of the tag
function tree:starttag(tag)
	local node = {}
	if self.parseAttributes == true then
		node._attr=tag.attrs
	end

	--Table in the stack representing the tag being processed
	local current = self._stack[#self._stack]

	if current[tag.name] then
		table.insert(current[tag.name], node)
	else
		current[tag.name] = {node; n=1}
	end

	table.insert(self._stack, node)
end

---Parses an end tag.
-- @param tag a {name, attrs} table
-- where name is the name of the tag and attrs
-- is a table containing the atributtes of the tag
function tree:endtag(tag, s)
	--Table in the stack representing the tag being processed
	local current = self._stack[#self._stack]
	--Table in the stack representing the containing tag of the current tag
	local prev = self._stack[#self._stack-1]
	if not prev[tag.name] then
		error("XML Error - Unmatched Tag ["..s..":"..tag.name.."]\n")
	end
	if prev == self.root then
		-- Once parsing complete, recursively reduce tree
		self:reduce(prev, nil, nil)
	end

	local firstKey = getFirstKey(current)
	table.remove(self._stack)
end

---Parses a tag content.
-- @param t text to process
function tree:text(text)
	local current = self._stack[#self._stack]
	table.insert(current, text)
end

---Parses CDATA tag content.
tree.cdata = tree.text
tree.__index = tree
return tree
