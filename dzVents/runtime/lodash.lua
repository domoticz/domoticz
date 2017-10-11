---
-- lodash for lua
-- @module lodash
-- @author Ted Moghimi
-- @license MIT

local _ = {
    _VERSION = '0.02'
}

--- Array
-- @section Array

---
-- Creates an array of elements split into groups the length of size. 
-- If collection can’t be split evenly, the final chunk will be the 
-- remaining elements.
-- @usage local t = _.chunk({'x', 'y', 'z', 1, 2, 3, 4, true , false}, 4)
-- _.print(t)
-- --> {{"x", "y", "z", 1}, {2, 3, 4, true}, {false}}
-- 
-- @param array The array to process.
-- @param[opt=1] size The length of each chunk.
-- @return the new array containing chunks.
_.chunk = function (array, size)
    local t = {}
    local size = size == 0 and 1 or size or 1
    local c, i = 1, 1
    while true do   
        t[i] = {}
        for j = 1, size do
            _.push(t[i], array[c])
            c = c + 1
        end
        if _.gt(c, #array) then
            break
        end
        i = i + 1
    end
    return t
end

---
-- Creates an array with all falsey values removed. The values false, 
-- nil are falsey.
-- @usage local t = _.compact({'x', 'y', nil, 1, 2, 3, false, true , false})
-- _.print(t)
-- --> {"x", "y", 1, 2, 3, true}
--
-- @param array The array to compact
-- @return Returns the new array of filtered values
_.compact = function (array)
    local t = {}
    for k, v in pairs(array) do     
        if v then
            _.push(t, v)
        end
    end 
    return t
end


---
-- Creates an array of unique array values not included in the other 
-- provided arrays.
-- @usage _.print(_.difference({3, 1, 2, 9, 5, 9}, {4, 5}, {9, 1}))
-- --> {3, 2}
--
-- @param array The array to inspect.
-- @param ... The arrays of values to exclude.
-- @return Returns the new array of filtered values.
_.difference = function (array, ...)
    local t = {}
    local c = 1
    local tmp = _.table(...)
    for k, v in ipairs(array) do
        while not _.isNil(tmp[c]) do
            for j, v2 in ipairs(tmp[c]) do
                if v == v2 then goto doubleBreak end
            end
            c = c + 1
        end
        _.push(t, v)
        ::doubleBreak::
        c = 1
    end
    return t
end

---
-- Creates a slice of array with n elements dropped from the beginning.
-- @usage _.print(_.drop({1, 2, 3, 4, 5, 6}, 2))
-- --> {3, 4, 5, 6}
-- 
-- @param array The array to query.
-- @param[opt=1] n The number of elements to drop.
-- @return Returns the slice of array.
_.drop = function (array, n)
    local n = n or 1    
    return _.slice(array, n + 1)
end


local callIteratee = function (predicate, selfArg, ...) 
    local result
    local predicate = predicate or _.identity
    if selfArg then
        result = predicate(selfArg, ...)
    else
        result = predicate(...)
    end
    return result
end

---
-- Creates a slice of array with n elements dropped from the end.
-- @usage _.print(_.dropRight({1, 2, 3, 4, 5, 6}, 2))
-- --> {1, 2, 3, 4}
--
-- @param array The array to query.
-- @param[opt=1] n The number of elements to drop.
-- @return Returns the slice of array.
_.dropRight = function (array, n)   
    local n = n or 1
    return _.slice(array, nil, #array - n)
end


local dropWhile = function(array, predicate, selfArg, start, step, right)
    local t = {}
    local c = start
    while not _.isNil(array[c]) do
        ::cont::
        if #t == 0 and 
            callIteratee(predicate, selfArg, array[c], c, array) then
            c = c + step
            goto cont
        end
        if right then
            _.enqueue(t, array[c])
        else
            _.push(t, array[c])
        end
        c = c + step            
    end 
    return t
end

---
-- Creates a slice of array excluding elements dropped from the end. 
-- Elements are dropped until predicate returns falsey.
-- @usage _.print(_.dropRightWhile({1, 5, 2, 3, 4, 5, 4, 4}, function(n)
--    return n > 3
-- end))
-- --> {1, 5, 2, 3}
--
-- @param array The array to query.
-- @param[opt=_.identity] predicate The function to invoked per iteratin.
-- @param[opt] selfArg The self binding of predicate.
-- @return Return the slice of array.
_.dropRightWhile = function(array, predicate, selfArg)
    return dropWhile(array, predicate, selfArg, #array, -1, true)
end

---
-- Creates a slice of array excluding elements dropped from the beginning. 
-- Elements are dropped until predicate returns falsey.
-- @usage _.print(_.dropWhile({1, 2, 2, 3, 4, 5, 4, 4, 2}, function(n)
--    return n < 3
-- end))
-- --> {3, 4, 5, 4, 4, 2}
--
-- @param array The array to query.
-- @param[opt=_.idenitity] predicate The function invoked per iteration.
-- @param[opt] selfArg The self binding of predicate.
-- @return Return the slice of array.
_.dropWhile = function(array, predicate, selfArg)
    return dropWhile(array, predicate, selfArg, 1, 1)
end

_.enqueue = function (array, value)
    return table.insert(array, 1, value)
end

---
-- Fills elements of array with value from start up to, including, end. 
-- @usage local array = {1, 2, 3, 4}
-- _.fill(array, 'x', 2, 3)
-- _.print(array)
-- --> {1, "x", "x", 4}
--
-- @param array The array to fill.
-- @param value The value to fill array with.
-- @param[opt=1] start The start position.
-- @param[opt=#array] stop The end position.
-- @return Returns array.
_.fill = function (array, value, start, stop)
    local start = start or 1
    local stop = stop or #array
    for i=start, stop, start > stop and -1 or 1 do
        array[i] = value
    end
    return  array
end


local findIndex = function(array, predicate, selfArg, start, step)
    local c = start
    while not _.isNil(array[c]) do
        if callIteratee(predicate, selfArg, array[c], c, array) then
            return c
        end
        c = c + step            
    end 
    return -1
end

---
-- This method is like [_.find](#_.find) except that it returns the index of the 
-- first element predicate returns truthy for instead of the element itself. 
-- @usage _.print(_.findIndex({{a = 1}, {a = 2}, {a = 3}, {a = 2}, {a = 3}}, function(v)
--     return v.a == 3
-- end))
-- --> 3
--
-- @param array The array to search.
-- @param[opt=_.idenitity] predicate The function invoked per iteration.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the index of the found element, else -1.
_.findIndex = function (array, predicate, selfArg)
    return findIndex(array, predicate, selfArg, 1, 1)
end

---
-- This method is like [_.findIndex](#_.findIndex) except that it iterates over 
-- elements of collection from right to left. 
-- @usage _.print(_.findLastIndex({{a = 1}, {a = 2}, {a = 3}, {a = 2}, {a = 3}}, function(v)
--     return v.a == 3
-- end))
-- --> 5
--
-- @param array The array to search.
-- @param[opt=_.idenitity] predicate The function invoked per iteration.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the index of the found element, else -1.
_.findLastIndex = function (array, predicate, selfArg)
    return findIndex(array, predicate, selfArg, #array, -1)
end


---
-- Gets the first element of array.
-- @usage _.print(_.first({'w', 'x', 'y', 'z'}))
-- --> w
--
-- @param array The array to query.
-- @return Returns the first element of array.
_.first = function (array)
    return array[1]
end

---
-- Flattens a nested array. 
-- If isDeep is true the array is recursively flattened, otherwise 
-- it’s only flattened a single level.
-- @usage _.print(_.flatten({1, 2, {3, 4, {5, 6}}}))
-- --> {1, 2, 3, 4, {5, 6}}
-- _.print(_.flatten({1, 2, {3, 4, {5, 6}}}, true))
-- --> {1, 2, 3, 4, 5, 6}
--
-- @param array The array to flatten.
-- @param isDeep Specify a deep flatten
-- @return Returns the new flattened array.
_.flatten = function(array, isDeep)
    local t = {}
    for k, v in ipairs(array) do
        if _.isTable(v) then
            local childeren
            if isDeep then
                childeren = _.flatten(v)
            else
                childeren = v               
            end
            for k2, v2 in ipairs(childeren) do
                _.push(t, v2) 
            end
        else
            _.push(t, v)
        end
    end
    return t
end

---
-- Recursively flattens a nested array.
-- @usage _.print(_.flattenDeep({1, 2, {3, 4, {5, 6}}}))
-- --> {1, 2, 3, 4, 5, 6}
--
-- @param array The array to flatten.
-- @return Returns the new flattened array.
_.flattenDeep = function (array)
    return _.flatten(array, true)
end

---
-- Gets the index at which the first occurrence of value is found in array.
-- @usage _.print(_.indexOf({2, 3, 'x', 4}, 'x'))
-- --> 3
-- 
-- @param array The array to search.
-- @param value The value to search for.
-- @param[opt=1] fromIndex The index to search from.
-- @return  Returns the index of the matched value, else -1.
_.indexOf = function (array, value, fromIndex)    
    return _.findIndex(array, function(n) 
        return n == value
    end)
end
-- 

---
-- Gets all but the last element of array.
-- @usage _.print(_.initial({1, 2, 3, 'a'}))
-- --> {1, 2, 3}
--
-- @param array The array to query.
-- @return Returns the slice of array.
_.initial = function (array)
    return _.slice(array, nil, #array - 1)
end
-- 

---
-- Creates an array of unique values that are included in all of the 
-- provided arrays.
-- @usage _.print(_.intersection({1, 2}, {4, 2}, {2, 1}))
-- --> {2}
--
-- @param The arrays to inspect.
-- @return Returns the new array of shared values.
_.intersection = function (...)
    local tmp = _.table(...)
    local first = table.remove(tmp, 1)
    local t = {}
    for i, v in ipairs(first) do
        local notFound = false
        for i2, v2 in ipairs(tmp) do
            if _.indexOf(v2, v) == -1 then
                notFound = true
                break
            end
        end
        if not notFound then 
            _.push(t, v)
        end
    end
    return t
    -- body
end


---
-- Gets the last element of array.
-- @usage _.print(_.last({'w', 'x', 'y', 'z'}))
-- --> z
--
-- @param array The array to query.
-- @return Returns the last element of array.
_.last = function(array)
    return array[#array]
end

---
-- This method is like [_.indexOf](#_.indexOf) except that it iterates
--  over elements of array from right to left.
-- @usage _.print(_.lastIndexOf({2, 3, 'x', 4, 'x', 5}, 'x'))
-- --> 5
--
-- @param array The array to search.
-- @param value The value to search for.
-- @param[opt=#array] fromIndex The index to search from.
-- @return  Returns the index of the matched value, else -1.
_.lastIndexOf = function (array, value, fromIndex)
   return _.findLastIndex(array, function(n) 
        return n == value
    end)
end


---
-- Removes all provided values from array.
-- @usage local array = {1, 2, 3, 4, 5, 4, 1, 2, 3}
-- _.pull(array, 2, 3)
-- _.print(array)
-- --> {1, 4, 5, 4, 1}
-- @param array The array to modify.
-- @param ... The values to remove.
-- @return Returns array
_.pull = function(array, ...)
    local i = 1
    while not _.isNil(array[i]) do
        for k, v in ipairs(_.table(...)) do
            if array[i] == v then 
                table.remove(array, i)
                goto cont
            end
        end
        i = i + 1
        ::cont::
    end 
    return array
end

_.push = function (array, value)
    return table.insert(array, value)
end

---
-- Removes elements from array corresponding to the given indexes and 
-- returns an array of the removed elements. Indexes may be specified 
-- as an array of indexes or as individual arguments. 
-- @usage local array = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'}
-- local t = _.pullAt(array, 4, 9, 8)
-- _.print(array)
-- --> {"a", "b", "c", "e", "f", "g", "j"}
-- _.print(t)
-- --> {"d", "h", "i"}
--
-- @param array The array to modify.
-- @param ... The indexes of elements to remove
-- @return Returns the new array of removed elements.
_.pullAt = function (array, ...)
    local t = {}
    local tmp = _.table(...)
    table.sort(tmp, function(a, b)
        return _.gt(a, b)
    end)    
    for i, index in ipairs(tmp) do
        _.enqueue(t, table.remove(array, index))
    end
    return t
end

---
-- Removes all elements from array that predicate returns truthy for 
-- and returns an array of the removed elements.
-- @usage local array = {1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 1, 2, 3, 5, 4}
-- local t = _.remove(array, function(value)
--     return value > 4
-- end)
-- _.print(array)
-- --> {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4}
-- _.print(t)
-- --> {5, 6, 7, 5, 6, 5}
--
-- @param array The array to modify.
-- @param predicate The function invoked per iteration.
-- @return Returns the new array of removed elements
_.remove = function(array, predicate)
    local t = {}
    local c = 1
    local predicate = predicate or _.identity
    while not _.isNil(array[c]) do
        if predicate(array[c], c, array) then
            _.push(t, table.remove(array, c))
            goto cont
        end        
        c = c + 1
        ::cont::
    end 
    return t
end


---
-- Gets all but the first element of array.
-- @usage _.print(_.rest({1, 2, 3, 'a'}))
-- --> {2, 3, 'a'}
-- @param array The array to query.
-- @return Returns the slice ofa array.
_.rest = function (array)
    return _.slice(array, 2, #array)
end


---
-- Reverses the array so the first element becomes the last, the second 
-- element becomes the second to last, and so on. 
-- @usage _.print(_.reverse({1, 2, 3, 'a', 'b'}))
-- --> {'b', 'a', 3, 2, 1}
--
-- @param array The array to mutate.
-- @return Returns the new reversed array.
_.reverse = function (array)
    local t = {}
    for i, v in ipairs(array) do
        _.enqueue(t, v)
    end
    return t
end


---
-- Creates a slice of array from start up to, including, end. 
-- @usage _.print(_.slice({1, 2, 3, 4, 5, 6}, 2, 3))
-- --> {2, 3}
-- 
-- @param array The array to slice.
-- @param[opt=1] start The start position.
-- @param[opt=#array] stop The end position
-- @return Returns the slice of array.
_.slice = function (array, start, stop)
    local start = start or 1
    local stop = stop or #array
    local t = {}    
    for i=start, stop do
        t[i - start + 1] = array[i]     
    end 
    return t
end

---
-- Creates a slice of array with n elements taken from the beginning.
-- @usage _.print(_.take({1, 2, 3, 4, 5}, 3))
-- --> {1, 2, 3}
--
-- @param array The array to query.
-- @param[opt=1] n The number of elements to take.
-- @return Returns the slice of array.
_.take = function(array, n)
    local n = n or 1
    return _.slice(array, 1, n)
end


---
-- Creates a slice of array with n elements taken from the end.
-- @usage _.print(_.takeRight({1, 2, 3, 4, 5}, 3))
-- --> {3, 4, 5}
-- 
-- @param array The array to query.
-- @param[opt=1] n The number of elements to take.
-- @return Returns the slice of array.
_.takeRight = function (array, n)
    local n = n or 1
    return _.slice(array, #array - n +1)
end

local takeWhile = function(array, predicate, selfArg, start, step, right)
    local t = {}
    local c = start
    while not _.isNil(array[c]) do
        if callIteratee(predicate, selfArg, array[c], c, array) then
            if right then
                _.enqueue(t, array[c])
            else
                _.push(t, array[c])
            end
        else
            break
        end
        c = c + step            
    end 
    return t
end

---
-- Creates a slice of array with elements taken from the end. Elements
-- are taken until predicate returns falsey. The predicate is bound to 
-- selfArg and invoked with three arguments: (value, index, array). 
-- @usage _.print(_.takeRightWhile({1, 2, 3, 4, 5, 6, 7, 8}, function(n)
--     return n > 4
-- end))
-- --> {5, 6, 7, 8}
--
-- @param array The array to query.
-- @param predicate The function invoked per iteration.
-- @param selfArg The self binding of predicate.
_.takeRightWhile = function (array, predicate, selfArg)
    return takeWhile(array, predicate, selfArg, #array, -1, true)
end

---
-- Creates an array of unique values, from all of the 
-- provided arrays.
-- @usage _.print(_.union({1, 2}, {4, 2}, {2, 1}))
-- --> {1, 2, 4}
--
-- @param ... The arrays to inspect
_.union = function (...)
    local tmp = _.table(...)
    local t = {}    
    for i, array in ipairs(tmp) do
        for i2, v in ipairs(array) do
            if _.indexOf(t, v) == -1 then                
                _.push(t, v)
            end
        end
    end
    return t
end



---
-- Creates a slice of array with elements taken from the beginning. Elements
-- are taken until predicate returns falsey. The predicate is bound to 
-- selfArg and invoked with three arguments: (value, index, array). 
-- @usage _.print(_.takeWhile({1, 2, 3, 4, 5, 6, 7, 8}, function(n)
--     return n < 5
-- end))
-- --> {1, 2, 3, 4}
--
-- @param array The array to query.
-- @param predicate The function invoked per iteration.
-- @param selfArg The self binding of predicate.
_.takeWhile = function (array, predicate, selfArg)
    return takeWhile(array, predicate, selfArg, 1, 1)
end

---
-- Creates a duplicate-free version of an array in which only the first 
-- occurence of each element is kept. If an iteratee function is provided 
-- it’s invoked for each element in the array to generate the criterion 
-- by which uniqueness is computed. The iteratee is bound to thisArg and 
-- invoked with three arguments: (value, index, array). 
-- @usage _.print(_.uniq({1, 3, 2, 2}))
-- --> {1, 3, 2}
--_.print(_.uniq({{x=1}, {x=2}, {x=2}, {x=3}, {x=1}}, function(n)
--     return n.x
-- end))
-- --> {{["x"]=1}, {["x"]=2}, {["x"]=3}}
--
-- @param array The array to inspect.
-- @param iteratee The function invoked per iteration.
-- @param selfArg The self binding of predicate.
-- @return Returns the new duplicate-value-free array.
_.uniq = function(array, iteratee, selfArg)
    local t = {}
    local results = {}
    for k, v in ipairs(array) do
        local r = callIteratee(iteratee, selfArg, v, k, array)
        if _.indexOf(results, r) == -1 then
            _.push(t, v)
        end
        _.push(results, r)
    end
    return t
end

---
-- The inverse of _.pairs; this method returns an object composed from
-- arrays of property names and values. Provide either a single two dimensional 
-- array, e.g. [[key1, value1], [key2, value2]] or two arrays, one of
-- property names and one of corresponding values.
-- @usage _.print(_.zipObject({{'fred', 30}, {'alex', 20}}))
-- --> {["alex"]=20, ["fred"]=30}
-- _.print(_.zipObject({'fred', 'alex'}, {30, 20}))
-- --> {["alex"]=20, ["fred"]=30}
--
-- @param ... The properties/values
-- @return Returns the new object.
_.zipObject = function (...)
    local tmp = _.table(...)
    local t = {}
    if #tmp == 1 then
        for i, pair in ipairs(tmp[1]) do
            t[pair[1]] = pair[2]
        end
    else
        for i = 1, #tmp[1] do
            t[tmp[1][i]] = tmp[2][i]
        end
    end
    return t
end


---
-- This method is like [_.zip](#_.zip) except that it accepts an array of grouped 
-- elements and creates an array regrouping the elements to their pre-zip 
-- configuration.
-- @usage local t = _.zip({'a', 'b', 'c'}, {1, 2, 3}, {10, 20, 30})
-- _.print(t)
-- --> {{"a", 1, 10}, {"b", 2, 20}, {"c", 3, 30}}
-- _.print(_.unzip(t))
-- --> {{"a", "b", "c"}, {1, 2, 3}, {10, 20, 30}}
--
-- @param array The array of grouped elements to process.
-- @return Returns the new array of regrouped elements.
_.unzip = function (array)
    return _.zip(_.args(array))
end

---
-- Creates an array excluding all provided values 
-- @usage _.print(_.without({1,1, 2, 3, 2, 3, 5, 5, 1, 2},  5, 1))
-- --> {2, 3, 2, 3, 2}
--
-- @param array The array to filter.
-- @param ... The values to exclude.
-- @return Returns the new array of filtered values.
_.without = function (array, ...)
    local t = {}
    for i, v in ipairs(array) do
        local args = _.table(...)
        if _.indexOf(args, v) == -1 then
            _.push(t, v)
        end
    end
    return t
end

---
-- Creates an array of grouped elements, the first of which contains 
-- the first elements of the given arrays, the second of which contains 
-- the second elements of the given arrays, and so on.
-- @usage local t = _.zip({'a', 'b', 'c'}, {1, 2, 3}, {10, 20, 30})
-- _.print(t)
-- --> {{"a", 1, 10}, {"b", 2, 20}, {"c", 3, 30}}
--
-- @param ... The arrays to process
-- @return Returns the new array of grouped elements.
_.zip = function (...)
    local t = {}
    for i, array in ipairs(_.table(...)) do
        for j, v in ipairs(array) do  
            t[j] = t[j] or {}
            t[j][i] = v
        end
    end
    return t
end

--- Collection
-- @section Collection

---
-- Creates an array of elements corresponding to the given keys, 
-- or indexes, of collection. Keys may be specified as individual 
-- arguments or as arrays of keys.
-- @usage _.print(_.at({'1', '2', '3', '4', a='a', b='b'}, {1, 2}, 'b'))
-- --> {"1", "2", "b"}
--
-- @param collection The collection to iterate over.
-- @param ... The property names or indexes of elements to pick, 
-- specified individually or in arrays.
-- @return Return the new array of picked elements.
_.at = function (collection, ...)
    local t = {}
    for k, key in ipairs(_.table(...)) do
        if _.isTable(key) then
            for k, key in ipairs(key) do
                _.push(t, collection[key])
            end
        else
            _.push(t, collection[key])
        end
    end
    return t
end

---
-- Creates an object composed of keys generated from the results of 
-- running each element of collection through iteratee. The corresponding 
-- value of each key is the number of times the key was returned by 
-- iteratee. The iteratee is bound to selfArg and invoked with three arguments:
-- (value, index|key, collection). 
--
-- @usage _.print(_.countBy({4.3, 6.1, 6.4}, function(n) 
--   return math.floor(n)
-- end))
-- --> {[4]=1, [6]=2}
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] iteratee The function invoked per iteration.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the composed aggregate object.
_.countBy = function (collection, iteratee, selfArg)
    local t = {}
    for k, v in _.iter(collection) do
        local r = _.str(
            callIteratee(iteratee, selfArg, v, k, collection)
        )
        if _.isNil(t[r]) then
            t[r] = 1
        else
            t[r] = t[r] + 1
        end
    end
    return t
end

---
-- Creates an object composed of keys generated from the results of 
-- running each element of collection through iteratee. The corresponding 
-- value of each key is an array of the elements responsible for generating 
-- the key. The iteratee is bound to selfArg and invoked with three arguments:
-- (value, index|key, collection). 
-- @usage _.print(_.groupBy({4.3, 6.1, 6.4}, function(n) 
--   return math.floor(n)
-- end))
-- --> {[4]={4.3}, [6]={6.1, 6.4}}
-- 
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] iteratee The function invoked per iteration.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the composed aggregate object.
_.groupBy = function (collection, iteratee, selfArg)
    local t = {}
    for k, v in _.iter(collection) do
        local r = _.str(
            callIteratee(iteratee, selfArg, v, k, collection)
        )
        if _.isNil(t[r]) then
            t[r] = {v}
        else
            _.push(t[r], v)
        end
    end
    return t
end

---
-- Creates an object composed of keys generated from the results of 
-- running each element of collection through iteratee. The corresponding 
-- value of each key is the last element responsible for generating the key. 
-- The iteratee function is bound to selfArg and invoked with three arguments:
-- (value, index|key, collection). 
-- @usage local keyData = {
--     {dir='l', a=1},
--     {dir='r', a=2}
-- }
-- _.print('40.indexBy          :', _.indexBy(keyData, function(n)
--     return n.dir
-- end))
-- --> {["l"]={[a]=1, [dir]="l"}, ["r"]={[a]=2, [dir]="r"}}
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] iteratee The function invoked per iteration.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the composed aggregate object.
_.indexBy = function (collection, iteratee, selfArg)
    local t = {}
    for k, v in _.iter(collection) do
        local r = _.str(
            callIteratee(iteratee, selfArg, v, k, collection)
        )
        t[r] = v
    end
    return t
end

---
-- Checks if predicate returns truthy for all elements of collection. 
-- The predicate is bound to selfArg and invoked with three arguments:
-- (value, index|key, collection). 
-- @usage _.print(_.every({1, 2, 3, 4, '5', 6}, _.isNumber))
-- --> false
-- _.print(_.every({1, 2, 3, 4, 5, 6}, _.isNumber))
-- --> true
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
_.every = function (collection, predicate, selfArg)
    for k, v in _.iter(collection) do
        if not callIteratee(predicate, selfArg, v, k, collection) then
            return false
        end
    end
    return true
end

local filter = function(collection, predicate, selfArg, reject)
    local t = {}
    for k, v in _.iter(collection) do
        local check = callIteratee(predicate, selfArg, v, k, collection)
        if reject then 
            if not check then
                _.push(t, v)
            end     
        else
            if check then
                _.push(t, v)
            end         
        end         
    end
    return t
end

---
-- Iterates over elements of collection, returning an array of all 
-- elements predicate returns truthy for. The predicate is bound to 
-- selfArg and invoked with three arguments: (value, index|key, collection). 
-- @usage _.print(_.filter({1, 2, 3, 4, '5', 6, '7'}, _.isNumber))
-- --> {1, 2, 3, 4, 6}
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
_.filter = function (collection, predicate, selfArg)
    return filter(collection, predicate, selfArg)
end

---
-- Iterates over elements of collection invoking iteratee for each element.
-- The iteratee is bound to selfArg and invoked with three arguments:
-- (value, index|key, collection). Iteratee functions may exit iteration 
-- early by explicitly returning false. 
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns collection.
_.forEach = function (collection, predicate, selfArg)
    for k, v in _.iter(collection) do
        callIteratee(predicate, selfArg, v, k, collection)
    end
    return collection
end

---
-- This method is like [_.forEach](#_.forEach) except that it iterates 
-- over elements of collection from right to left.
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns collection.
_.forEachRight = function (collection, predicate, selfArg)
    for k, v in _.iterRight(collection) do
        callIteratee(predicate, selfArg, v, k, collection)
    end
    return collection
end

---
-- Checks if target is in collection. 
-- @usage print(_.includes({1, 2, 'x', 3, ['5']=4, x=3, 5}, 'x'))
-- --> true
-- print(_.includes({1, 2, 'x', 3, ['5']=4, x=3, 5}, 'z'))
-- --> false
-- @param collection The collection to search
-- @param target The value to search for.
_.includes = function (collection, target)
    local result = _.find(collection, function (n)
        return n == target
    end)
    return result ~= nil
end

---
-- Invokes method of each element in collection, returning an array of the 
-- results of each invoked method. Any additional arguments are provided 
-- to each invoked method. func bound to, each element in collection.
-- @usage _.print(_.invoke({'1.first', '2.second', '3.third'}, string.sub, 1, 1))
-- --> {"1", "2", "3"}
--
-- @param collection The collection to iterate over.
-- @param method The method to invoke per iteration.
-- @param ... The arguments to invoke the method with.
-- @return Returns the array of results.
_.invoke = function (collection, method, ...)
    local t = {}
    for k, v in _.iter(collection) do
        _.push(t, callIteratee(method, v, ...))
    end
    return t
end

---
-- Creates an array of values by running each element in collection through
-- iteratee. The iteratee is bound to selfArg and invoked with three 
-- arguments: (value, index|key, collection). 
-- @usage _.print(_.map({1, 2, 3, 4, 5, 6, 7, 8, 9}, function(n)
--     return n * n
-- end))
-- --> {1, 4, 9, 16, 25, 36, 49, 64, 81}
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] iteratee The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
_.map = function (collection, iteratee, selfArg)
    local t = {}
    for k, v in _.iter(collection) do
        t[k] = callIteratee(iteratee, selfArg, v, k, collection)
    end
    return t
end

---
-- Creates an array of elements split into two groups, the first of 
-- which contains elements predicate returns truthy for, while the second 
-- of which contains elements predicate returns falsey for. The predicate 
-- is bound to selfArg and invoked with three arguments: 
-- (value, index|key, collection). 
-- @usage _.print(_.partition({1, 2, 3, 4, 5, 6, 7}, function (n)
--     return n > 3
-- end))
-- --> {{4, 5, 6, 7}, {1, 2, 3}}
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
-- @return  Returns the array of grouped elements.
_.partition = function (collection, predicate, selfArg)
    local t = {{}, {}}
    for k, v in _.iter(collection) do
        if callIteratee(predicate, selfArg, v, k, collection) then
            _.push(t[1], v)
        else
            _.push(t[2], v)
        end
    end
    return t
end

--- 
-- Gets the property value of path from all elements in collection.
-- @usage local users = {
--   { user = 'barney', age = 36, child = {age = 5}},
--   { user = 'fred',   age = 40, child = {age = 6} }
-- }
-- _.print(_.pluck(users, {'user'}))
-- --> {"barney", "fred"}
-- _.print(_.pluck(users, {'child', 'age'}))
-- --> {5, 6}
--
-- @param collection The collection to iterate over.
-- @param path The path of the property to pluck.
_.pluck = function (collection, path)
    local t = {}
    for k, value in _.iter(collection) do 
        _.push(t, _.get(value, path))
    end
    return t
end


---
-- Reduces collection to a value which is the accumulated result of 
-- running each element in collection through iteratee, where each 
-- successive invocation is supplied the return value of the previous. 
-- If accumulator is not provided the first element of collection is used
--  as the initial value. The iteratee is bound to selfArg and invoked 
-- with four arguments: (accumulator, value, index|key, collection). 
-- @usage _.print(_.reduce({1, 2, 3}, function(total, n)
--   return n + total;
-- end))
-- --> 6
-- _.print(_.reduce({a = 1, b = 2}, function(result, n, key) 
--     result[key] = n * 3
--     return result;
-- end, {}))
-- --> {["a"]=3, ["b"]=6}
-- 
-- @param collection The collection to iterate over.
-- @param[opt=_.identity] iteratee The function invoked per iteration.
-- @param[opt=<first element>] accumulator The initial value.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the accumulated value.
_.reduce = function (collection, iteratee, accumulator, selfArg)
    local accumulator = accumulator
    for k, v in _.iter(collection) do 
        if _.isNil(accumulator) then 
            accumulator = v 
        else
            accumulator =  callIteratee(iteratee, selfArg, accumulator, v, k, collection)
        end
    end
    return accumulator
end

---
-- This method is like _.reduce except that it iterates over elements 
-- of collection from right to left.
-- @usage local array = {0, 1, 2, 3, 4, 5};
-- _.print(_.reduceRight(array, function(str, other) 
--   return str .. other
-- end, ''))
-- --> 543210
-- 
-- @param collection The collection to iterate over.
-- @param[opt=_.identity] iteratee The function invoked per iteration.
-- @param[opt=<first element>] accumulator The initial value.
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns the accumulated value.
_.reduceRight = function (collection, iteratee, accumulator, selfArg)
    local accumulator = accumulator
    for k, v in _.iterRight(collection) do 
        if _.isNil(accumulator) then 
            accumulator = v 
        else
            accumulator =  callIteratee(iteratee, selfArg, accumulator, v, k, collection)
        end
    end
    return accumulator
end

---
-- The opposite of [_.filter](#_.filter); this method returns the elements of 
-- collection that predicate does not return truthy for.
-- @usage _.print(_.reject({1, 2, 3, 4, '5', 6, '7'}, _.isNumber))
-- --> {"5", "7"}
--
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
_.reject = function (collection, predicate, selfArg)
    return filter(collection, predicate, selfArg, true)
end

---
-- Gets a random element or n random elements from a collection.
-- @usage _.print(_.sample({1, 2, 3, a=4, b='x', 5, 23, 24}, 4))
-- --> {5, "x", 1, 23}
-- _.print(_.sample({1, 2, 3, a=4, b='x', 5, 23, 24}))
-- --> 4
-- 
-- @param collection The collection to sample.
-- @param[opt=1] n The number of elements to sample.
-- @return Returns the random sample(s).
_.sample = function (collection, n)
    local n = n or 1
    local t = {}
    local keys = _.keys(collection)
    for i=1, n do        
        local pick = keys[_.random(1, #keys)]
        _.push(t, _.get(collection, {pick}))
    end
    return #t == 1 and t[1] or t
end

---
-- Gets the size of collection by returning its length for array-like 
-- values or the number of own enumerable properties for objects.
-- @usage _.print(_.size({'abc', 'def'}))
-- --> 2
-- _.print(_.size('abcdefg'))
-- --> 7
-- _.print(_.size({a=1, b=2,c=3}))
-- --> 3
--
-- @param collection The collection to inspect.
-- @return Returns the size of collection.
_.size = function (collection)
    local c = 0
    for k, v in _.iter(collection) do
        c = c + 1
    end
    return c
end

---
-- Checks if predicate returns truthy for any element of collection. 
-- The function returns as soon as it finds a passing value and does 
-- not iterate over the entire collection. The predicate is bound to 
-- selfArg and invoked with three arguments: (value, index|key, collection). 
--
-- @usage _.print(_.some({1, 2, 3, 4, 5, 6}, _.isString))
-- --> false
-- _.print(_.some({1, 2, 3, 4, '5', 6}, _.isString))
-- --> true
-- @param collection The collection to iterate over. (table|string)
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
-- @return Returns true if any element passes the predicate check, else false.
_.some = function (collection, predicate, selfArg)
    for k, v in _.iter(collection) do
        if callIteratee(predicate, selfArg, v, k, collection) then
            return true
        end
    end
    return false
end

---
-- Creates an array of elements, sorted in ascending order by the 
-- results of running each element in a collection through iteratee. 
-- The iteratee is bound to selfArg and 
-- invoked with three arguments: (value, index|key, collection). 
-- @usage local t = {1, 2, 3}
-- _.print(_.sortBy(t, function (a)
--     return math.sin(a)
-- end))
-- --> {1, 3, 2}
-- local users = {
--     { user='fred' },
--     { user='alex' },
--     { user='zoee' },
--     { user='john' },
-- }
-- _.print(_.sortBy(users, function (a)
--     return a.user
-- end))
-- --> {{["user"]="alex"}, {["user"]="fred"}, {["user"]="john"}, {["user"]="zoee"}}
-- 
-- @param collection The collection to iterate over.
-- @param[opt=_.identity] predicate The function invoked per iteration
-- @param[opt] selfArg The self binding of predicate.
-- @return  Returns the new sorted array.
_.sortBy = function (collection, predicate, selfArg)
    local t ={}
    local empty = true
    local previous
    for k, v in _.iter(collection) do 
        if empty then 
            _.push(t, v)
            previous = callIteratee(predicate, selfArg, v, k, collection)            
            empty = false
        else
            local r = callIteratee(predicate, selfArg, v, k, collection)
            if _.lt(previous, r) then
                table.insert(t, v)
                previous = r
            else
                table.insert(t, #t, v)
            end            
        end
    end
    return t
end

---
-- Iterates over elements of collection, returning the first element 
-- predicate returns truthy for. The predicate is bound to selfArg and
-- invoked with three arguments: (value, index|key, collection). 
-- @usage _.print(_.find({{a = 1}, {a = 2}, {a = 3}, {a = 2}, {a = 3}}, function(v)
--     return v.a == 3
-- end))
-- --> {[a]=3}
--
-- @param collection The collection to search. (table|string)
-- @param predicate The function invoked per iteration
-- @param selfArg The self binding of predicate.
_.find = function (collection, predicate, selfArg)
    for k, v in _.iter(collection) do
        if callIteratee(predicate, selfArg, v, k, collection) then
            return v
        end
    end
end

---
-- This method is like _.find except that it iterates over elements of
-- collection from right to left.
-- @usage _.findLast({{a = 1}, {a = 2}, {a = 3, x = 1}, {a = 2}, {a = 3, x = 2}}, 
-- function(v)
--     return v.a == 3
-- end))
-- --> {[a]=3, [x]=2}
--
-- @param collection The collection to search. (table|string)
-- @param predicate The function invoked per iteration
-- @param selfArg The self binding of predicate.
_.findLast = function (collection, predicate, selfArg)
    for k, v in _.iterRight(collection) do
        if callIteratee(predicate, selfArg, v, k, collection) then
            return v
        end
    end
end

--- Function
-- @section Function

---
-- This method creates a function that invokes func once it’s called n
--  or more times.
-- @usage local printAfter3 = _.after(3, print)
-- for i = 1, 5 do
--    printAfter3('done', i)    
-- end 
-- --> done 4
-- --> done 5
-- 
-- @param n The number of calls before func invoked.
-- @param func The function to restrict.
-- @return Returns the new restricted function.
_.after = function(n, func)
    local i = 1
    return function(...)
        if _.gt(i, n) then
            return func(...)
        end
        i = i + 1
    end
end

---
-- Creates a function that accepts up to n arguments ignoring any 
-- additional arguments.
-- @usage local printOnly3 =_.ary(print, 3)
-- printOnly3(1, 2, 3, 'x', 'y', 6)
-- --> 1    2   3
--
-- @param func The function to cap arguments for.
-- @param n the arity cap.
-- @return Returns the new function
_.ary = function(func, n)
    return function(...)
        if n == 1 then
            return func((...))
        else
            local t = _.table(...)
            local first = _.take(t, n)
            return func(_.args(first))
        end
    end
end

---
-- Creates a function that invokes func while it’s called less than n times.
-- Subsequent calls to the created function return the result of the 
-- last func invocation.
-- @usage local printBefore3 = _.before(3, print)
-- for i = 1, 10 do
--     printBefore3(i, 'ok')
-- end
-- -->  1   ok
-- -->  2   ok
-- -->  3   ok
--
-- @param n The number of calls at which func is no longer invoked.
-- @param func The function to restrict.
-- @return Returns the new restriced function.
_.before = function (n, func)
    local i = 1
    local result
    return function (...)
        if _.lte(i, n) then
            result = func(...)  
        end
        i = i + 1
        return result
    end
end

---
-- Creates a function that runs each argument through a corresponding 
-- transform function.
-- @usage local increment = function(...) 
--     return _.args(_.map(_.table(...), function(n)
--         return n + 1
--     end))
-- end
-- local pow = function(...) 
--     return _.args(_.map(_.table(...), function(n)
--         return n * n
--     end))
-- end
-- local modded = _.modArgs(function(...)
--     print(...)
-- end, {increment, increment}, pow)
-- modded(0, 1, 2)
-- -->  4   9   16
--
-- @param func The function to wrap
-- @param ... The functions to transform arguments, specified as 
-- individual functions or arrays of functions.
-- @return Returns the new function.
_.modArgs = function (func, ...)
    local transforms = {}
    for i, v in ipairs( _.table(...)) do
        if _.isFunction(v) then 
            _.push(transforms, v) 
        elseif _.isTable(v) then
            for k2, v2 in _.iter(v) do
                if _.isFunction(v2) then _.push(transforms, v2) end    
            end
        end
    end
    return function(...)
        local args
        for i, transform in ipairs(transforms) do
            if _.isNil(args) then
                args = _.table(transform(...))
            else
                args = _.table(transform(_.args(args)))
            end
        end
        if _.isNil(args) then
            return func(...)
        else
            return func(_.args(args))
        end
    end
end

---
-- Creates a function that negates the result of the predicate func. 
-- The func predicate is invoked with arguments of the created function.
-- @usage local isEven = function (n)
--     return n % 2 == 0
-- end
-- local isOdd = _.negate(isEven)
-- _.print(_.filter({1, 2, 3, 4, 5, 6}, isEven))
-- --> {2, 4, 6}
-- _.print(_.filter({1, 2, 3, 4, 5, 6}, isOdd))
-- --> {1, 3, 5}
-- 
-- @param func The preadicate to negate.
-- @return Returns the new function
_.negate = function (func)
    return function(...)
        return not func(...)
    end
end

---
-- Creates a function that is restricted to invoking func once. Repeat
-- calls to the function return the value of the first call. The func
-- is invoked with arguments of the created function.
-- @usage local createApp = function(version)
--     print('App created with version '..version)
--     return version
-- end
-- local initialize = _.once(createApp)
-- initialize(1.1)
-- initialize(1.1)
-- initialize(1.1)
-- --> App created with version 1.1
-- --> 1.1
-- --> 1.1
-- --> 1.1
--
-- @param func The function to restrict.
-- @return Returns the new function.
_.once = function (func)
    local called = false; 
    local result
    return function(...)
        if not called then
            result = func(...)
            called = true            
        end
        return result
    end
end


---
-- Creates a function that invokes func with arguments arranged according 
-- to the specified indexes where the argument value at the first index 
-- is provided as the first argument, the argument value at the second 
-- index is provided as the second argument, and so on.
-- @usage local rearged = _.rearg(function(a, b, c) 
--   return {a, b, c};
-- end, 2, 1, 3)
-- _.print(rearged('a', 'b', 'c'))
-- --> {"b", "a", "c"}
-- _.print(rearged('b', 'a', 'c'))
-- --> {"a", "b", "c"}
--
-- @param func The function to rearrange arguments for.
-- @param ... The arranged argument indexes, specified as individual 
-- indexes or arrays of indexes.
-- @return Returns the new function.
_.rearg = function (func, ...)
    local indexes = {}
    for i, v in ipairs(_.table(...)) do
        if _.isNumber(v) then 
            _.push(indexes, v) 
        elseif _.isTable(v) then
            for k2, v2 in _.iter(v) do
                if _.isNumber(v2) then _.push(indexes, v2) end    
            end
        end
    end
    return function(...)
        local args = _.table(...)
        local newArgs = {}
        for i, index in ipairs(indexes) do  
            _.push(newArgs, args[index]) 
        end
        if #indexes == 0 then
            return func(...)
        else
            return func(_.args(newArgs))
        end
    end
end



--- Lang
-- @section Lang
---

---
-- Cast value to arguments
-- @usage print(_.args({1, 2, 3}))
-- --> 1    2   3
--
-- @param value value to cast
-- @return Returns arguments
_.args = function (value)
    if _.isTable(value) then return table.unpack(value) 
    else return table.unpack({value})
    end
end

---
-- Checks if value is greater than other.
-- @usage _.print(_.gt(1, 3))
-- --> false
-- _.print(_.gt(4, 3))
-- --> true
--
-- @param value The value to compare.
-- @param other The other value to compare.
_.gt = function (value, other, ...)
    local value, other = _.cast(value, other)
    if _.isString(value) or _.isNumber(value) then 
        return value > other
    elseif _.isFunction(value) then
        return value(...) > other(...)
    end
    return false
end

---
-- Checks if value is greater than other.
-- @usage _.print(_.gte(1, 3))
-- --> false
-- _.print(_.gte(3, 3))
-- --> true
--
-- @param value The value to compare.
-- @param other The other value to compare.
_.gte = function (value, other, ...)    
    if _.isNil(value) or _.isBoolean(value) then 
        return value == other
    end
    local value, other = _.cast(value, other)
    if _.isString(value) or _.isNumber(value) then 
        return value >= other
    elseif _.isFunction(value) then
        return value(...) >= other(...)
    elseif _.isTable(value) then
        return false 
    end
    return false
end

---
-- Checks if value is classified as a boolean primitive.
-- @usage _.print(_.isBoolean(false))
-- --> true
-- _.print(_.isBoolean('x'))
-- --> false
--
-- @param value the value to check
-- @return Returns true if value is correctly classified, else false.
_.isBoolean = function(value)
    return type(value) == 'boolean'
end

---
-- Checks if value is empty. A value is considered empty unless it’s an
-- arguments table, array, string with a length greater than 0 or an 
--object with own enumerable properties.
--@usage _.print(_.isEmpty(true))
-- --> true
-- _.print(_.isEmpty(1))
-- --> true
-- _.print(_.isEmpty({1, 2, 3}))
-- --> false
-- _.print(_.isEmpty({a= 1}))
-- --> false
--
-- @param value The value to inspect.
-- @return Returns true if value is empty, else false.
_.isEmpty = function (value)
    if _.isString(value) then
        return #value == 0
    elseif _.isTable(value) then
        local i = 0
        for k, v in _.iter(value) do
            i = i + 1
        end
        return i == 0
    else
        return true
    end
end

---
-- Checks if value is classified as a function primitive.
-- @usage _.print(_.isFunction(function() end))
-- --> true
-- _.print(_.isFunction(1))
-- --> false
--
-- @param value the value to check
-- @return Returns true if value is correctly classified, else false.
_.isFunction = function(value)
    return type(value) == 'function'
end

---
-- Checks if value is classified as a nil primitive.
-- @usage _.print(_.isNil(variable)
-- --> true
-- variable = 1
-- _.print(_.isNil(variable))
-- --> false
--
-- @param value the value to check
-- @return Returns true if value is correctly classified, else false.
_.isNil = function(value)
    return type(value) == 'nil'
end

---
-- Checks if value is classified as a number primitive.
-- @usage _.print(_.isNumber(1))
-- --> true
-- _.print(_.isNumber('1'))
-- --> false
--
-- @param value the value to check
-- @return Returns true if value is correctly classified, else false.
_.isNumber = function(value)
    return type(value) == 'number'
end

---
-- Checks if value is classified as a string primitive.
-- @usage _.print(_.isString('1'))
-- --> true
-- _.print(_.isString(1))
-- --> false
--
-- @param value the value to check
-- @return Returns true if value is correctly classified, else false.
_.isString = function(value)
    return type(value) == 'string'
end

---
-- Checks if value is classified as a table primitive.
-- @usage _.print(_.isTable({'1'}))
-- --> true
-- _.print(_.isString(1))
-- --> false
--
-- @param value the value to check
-- @return Returns true if value is correctly classified, else false.
_.isTable = function(value)
    return type(value) == 'table'
end

-- local implicitCast function (...)
--     if 
-- end

---
-- Checks if value is less than other.
-- @usage _.print(_.lt(1, 3))
-- --> true
-- _.print(_.lt(3, 3))
-- --> false
--
-- @param value The value to compare.
-- @param other The other value to compare.
_.lt = function (value, other, ...)
    local value, other = _.cast(value, other)
    if _.isString(value) or _.isNumber(value) then 
        return value < other
    elseif _.isFunction(value) then
        return value(...) < other(...)
    end
    return false
end

---
-- Checks if value is less than or euqal to other.
-- @usage _.print(_.lt(4, 3))
-- --> false
-- _.print(_.lt(3, 3))
-- --> true
-- @param value The value to compare.
-- @param other The other value to compare.
_.lte = function (value, other, ...)    
    if _.isNil(value) or _.isBoolean(value) then 
        return value == other
    end
    local value, other = _.cast(value, other)
    if _.isString(value) or _.isNumber(value) then 
        return value <= other
    elseif _.isFunction(value) then
        return value(...) <= other(...)
    elseif _.isTable(value) then
        return false
    end
    return false
end

_.cast = function (a, b)
    if type(a) == type(b) then return a, b end
    local cast
    if _.isString(a) then cast = _.str
    elseif _.isBoolean(a) then cast = _.bool
    elseif _.isNumber(a) then cast = _.num
    elseif _.isFunction(a) then cast = _.func
    elseif _.isTable(a) then cast = _.table
    end
    return a, cast(b)
end

---
-- Cast parameters to a function that return passed parameters. 
-- @usage local f = _.func(1, 2, 3)
-- _.print(f())
-- --> 1    2   3
--
-- @param value value to cast
-- @param ... The parameters to pass to any detected function
-- @return casted value
_.func = function (...)
    local t = _.table(...)
    return function ()
        return _.args(t)
    end
end

---
-- Cast parameters to table using table.pack
-- @usage print(_.table(1, 2, 3))
-- --> {1, 2, 3}
-- print(_.table("123"))
-- --> {"123"}
-- print(_.table(0))
-- --> {0}
--
-- @param value value to cast
-- @param ... The parameters to pass to any detected function
-- @return casted value
_.table = function (...)
    return table.pack(...)
end

---
-- Cast anything to boolean. If any function detected, call and cast its
-- result. Return false for 0, nil, table and empty string. 
-- @usage print(_.bool({1, 2, 3}))
-- --> false
-- print(_.bool("123"))
-- --> true
-- print(_.bool(0))
-- --> false
-- print(_.bool(function(a) return a end, "555"))
-- --> true
--
-- @param value value to cast
-- @param ... The parameters to pass to any detected function
-- @return casted value
_.bool = function (value, ...)
    local bool = false
    if _.isString(value) then     
        bool = #value > 0
    elseif _.isBoolean(value) then
        bool = value
    elseif _.isNumber(value) then
        bool = value ~= 0
    elseif _.isFunction(value) then
        bool = _.bool(value(...))    
    end
    return bool
end

---
-- Cast anything to number. If any function detected, call and cast its
-- result. Return 0 for nil and table. 
-- @usage print(_.num({1, 2, 3}))
-- --> 0
-- print(_.num("123"))
-- --> 123
-- print(_.num(true))
-- --> 1
-- print(_.num(function(a) return a end, "555"))
-- --> 555
--
-- @param value value to cast
-- @param ... The parameters to pass to any detected function
-- @return casted value
_.num = function (value, ...)
    local num = 0
    if _.isString(value) then   
        ok = pcall(function()
            num = value + 0
        end)
        if not ok then
            num = math.huge
        end
    elseif _.isBoolean(value) then
        num = value and 1 or 0    
    elseif _.isNumber(value) then
        num = value
    elseif _.isFunction(value) then
        num = _.num(value(...))
    end
    return num 
end


local dblQuote = function (v)
    return '"'..v..'"'
end
   
---
-- Cast anything to string. If any function detected, call and cast its
-- result.
-- @usage print(_.str({1, 2, 3, 4, {k=2, {'x', 'y'}}}))
-- --> {1, 2, 3, 4, {{"x", "y"}, ["k"]=2}}
-- print(_.str({1, 2, 3, 4, function(a) return a end}, 5))
-- --> {1, 2, 3, 4, 5}
--
-- @param value value to cast
-- @param ... The parameters to pass to any detected function
-- @return casted value
_.str = function (value, ...)
    local str = '';
    -- local v;
    if _.isString(value) then     
        str = value
    elseif _.isBoolean(value) then
        str = value and 'true' or 'false'
    elseif _.isNil(value) then
        str = 'nil'
    elseif _.isNumber(value) then
        str = value .. ''
    elseif _.isFunction(value) then       
        str = _.str(value(...))
    elseif _.isTable(value) then
        str = '{'
        for k, v in pairs(value) do
            v = _.isString(v) and dblQuote(v) or _.str(v, ...)
            if _.isNumber(k) then
                str = str .. v .. ', '              
            else
                str = str .. '[' .. dblQuote(k) .. ']=' .. v .. ', '
            end
        end     
        str = str:sub(0, #str - 2) .. '}'
    end
    return str
end


--- Number
-- @section Number

---
-- Checks if n is between start and up to but not including, end. 
-- If end is not specified it’s set to start with start then set to 0.
-- @usage print(_.inRange(-3, -4, 8))
-- --> true
--
-- @param n The number to check.
-- @param start The start of the range.
-- @param stop The end of the range.
-- @return Returns true if n is in the range, else false.
_.inRange = function (n, start, stop)
    local _start = _.isNil(stop) and 0 or start or 0
    local _stop = _.isNil(stop) and start or stop or 1
    return n >= _start and n < _stop
end

---
-- Produces a random number between min and max (inclusive). 
-- If only one argument is provided a number between 0 and the given 
-- number is returned. If floating is true, a floating-point number is
-- returned instead of an integer.
-- @usage _.print(_.random())
-- --> 1
-- _.print(_.random(5))
-- --> 3
-- _.print(_.random(5, 10, true))
-- --> 8.8120200577248
--
-- @param[opt=0] min the minimum possible value.
-- @param[opt=1] max the maximum possible value.
-- @param[opt=false] floating Specify returning a floating-point number.
-- @return Returns the random number.
_.random = function (min, max, floating)
    local minim = _.isNil(max) and 0 or min or 0
    local maxim = _.isNil(max) and min or max or 1
    math.randomseed(os.clock() * math.random(os.time()))
    local r = math.random(minim, maxim)
    if floating then
        r = r + math.random()
    end 
    return r
end

--- Object
-- @section Object

---
-- Gets the property value at path of object. If the resolved value 
-- is nil the defaultValue is used in its place.
-- @usage local object = {a={b={c={d=5}}}}
-- _.print(_.get(object, {'a', 'b', 'c', 'd'}))
-- --> 5
--
-- @param object The object to query.
-- @param path The path of the property to get.
-- @param[opt=nil] defaultValue The value returned if the resolved value is nil.
-- @return Returns the resolved value.
_.get = function (object, path, defaultValue)
    if _.isTable(object) then
        local value = object
        local c = 1
        while not _.isNil(path[c]) do
            if not _.isTable(value) then return defaultValue end
            value = value[path[c]]    
            c = c + 1
        end
        return value or defaultValue
    elseif _.isString(object) then
        local index = path[1]
        return object:sub(index, index)
    end
end

---
-- Checks if path is a direct property.
-- @usage local object = {a={b={c={d}}}}
-- print(_.has(object, {'a', 'b', 'c'}))
-- --> true
--
-- @param object The object to query
-- @param path The path to check (Array)
_.has = function (object, path)
    local obj = object
    local c = 1
    local exist = true
    while not _.isNil(path[c]) do
        obj = obj[path[c]]
        if _.isNil(obj) then
            exist = false
            break
        end
        c = c + 1
    end
    return exist   
end

---
-- This method is like _.find except that it returns the key of the 
-- first element predicate returns truthy for instead of the element itself. 
-- @usage _.print(_.findKey({a={a = 1}, b={a = 2}, c={a = 3, x = 1}}, 
-- function(v)
--     return v.a == 3
-- end))
-- --> c
--
-- @param object The collection to search. (table|string)
-- @param predicate The function invoked per iteration
-- @param selfArg The self binding of predicate.
-- @return Returns the key of the matched element, else nil
_.findKey = function (object, predicate, selfArg)
    for k, v in _.iter(object) do
        if callIteratee(predicate, selfArg, v, k, object) then
            return k
        end
    end
end

---
-- This method is like _.find except that it returns the key of the 
-- first element predicate returns truthy for instead of the element itself. 
-- @usage _.print(_.findLastKey({a={a=3}, b={a = 2}, c={a=3, x = 1}}, 
-- function(v)
--     return v.a == 3
-- end))
-- --> c
--
-- @param object The object to search. (table|string)
-- @param predicate The function invoked per iteration
-- @param selfArg The self binding of predicate.
-- @return Returns the key of the matched element, else nil
_.findLastKey = function (object, predicate, selfArg)
    for k, v in _.iterRight(object) do
        if callIteratee(predicate, selfArg, v, k, object) then
            return k
        end
    end
end

---
-- Creates an array of function property names from all enumerable 
-- properties, own and inherited, of object.
-- @usage _.print(_.functions(table))
-- --> {"concat", "insert", "maxn", "pack", "remove", "sort", "unpack"}
--
-- @param object The object to inspect.
-- @return Returns the new array of property names.
_.functions = function(object)
    local t = {}
    for k, v in _.iter(object) do
        if _.isFunction(v) then
            _.push(t, k)
        end
    end
    return t
end

---
-- Creates an object composed of the inverted keys and values of object. 
-- If object contains duplicate values, subsequent values overwrite 
-- property assignments of previous values unless multiValue is true.
-- @usage _.print(_.invert({a='1', b='2', c='3', d='3'}))
-- --> {[2]="b", [3]="d", [1]="a"}
-- _.print(_.invert({a='1', b='2', c='3', d='3'}, true))
-- --> {[2]="b", [3]={"c", "d"}, [1]="a"}
--
-- @param object The object to invert.
-- @param multiValue Allow multiple values per key.
-- @return Returns the new inverted object.
_.invert = function (object, multiValue)
    local t = {}
    for k, v in _.iter(object) do
        if multiValue and not _.isNil(t[v]) then
            t[v] = { t[v] }
            _.push(t[v], k)            
        else
            t[v] = k 
        end
    end
    return t
    
end

local getSortedKeys = function(collection, desc)
    local sortedKeys = {} 
    for k, v in pairs(collection) do
        table.insert(sortedKeys, k)
    end
    if desc then
        table.sort(sortedKeys, _.gt)
    else
        table.sort(sortedKeys, _.lt)
    end
    return sortedKeys
end

---
-- Creates an array of the own enumerable property names of object. 
-- @usage _.print(_.keys("test"))
-- --> {1, 2, 3, 4}
-- _.print(_.keys({a=1, b=2, c=3}))
-- --> {"c", "b", "a"}
-- 
-- @param object The object to query. (table|string)
-- @return Returns the array of property names.
_.keys = function (object)
    if _.isTable(object) then
        return getSortedKeys(object)
    elseif _.isString(object) then
        local keys = {}
        for i=1, #object do
            keys[i] = i
        end
        return keys
    end
end

--- 
-- Creates a two dimensional array of the key-value pairs for object.
-- @usage  _.print(_.pairs({1, 2, 'x', a='b'}))
-- --> {{1, 1}, {2, 2}, {3, "x"}, {"a", "b"}}
--
-- @param object The object to query
-- @return Returns the new array of key-value pairs.
_.pairs = function (object)
    local t = {}
    for k, v in _.iter(object) do
        _.push(t, {k, v})
    end
    return t
end

---
-- This method is like _.get except that if the resolved value is a
-- function it’s invoked with additional parameters and its result is returned.
-- @usage local object = {a=5, b={c=function(a) print(a) end}}
-- _.result(object, {'b', 'c'}, nil, 5)
-- --> 5
--
-- @param object The object to query.
-- @param path The path of the property to get.
-- @param[opt=nil] defaultValue The value returned if the resolved value is nil.
-- @param ... Additional parameters to pass to function
-- @return Returns the resolved value.
_.result = function (object, path, defaultValue, ...)
    local result = _.get(object, path, defaultValue)
    if _.isFunction(result) then 
        return result(...)
    else 
        return result
    end
end


---
-- Creates an array of the own enumerable property values of object. 
-- @usage _.print(_.values("test"))
-- --> {"t", "e", "s", "t"}
-- _.print(_.values({a=1, b=2, c=3}))
-- --> {1, 3, 2}
-- 
-- @param object The object to query. (table|string)
-- @return Returns the array of property values.
_.values = function (object)
    local t = {}
    for k, v in _.iter(object) do
        _.push(t, v)
    end
    return t
end

--- String
-- @section String


--- Utility
-- @section Utility

---
-- Creates a function that returns value.
-- @usage local object = {x=5}
-- local getter = _.constant(object)
-- _.print(getter() == object);
-- --> true
--
-- @param value Any value.
-- @return Returns the new function.
_.constant = function(value) 
    return _.func(value)
end

---
-- This method returns the first argument provided to it.
-- @usage local object = {x=5}
-- _.print(_.identity(object) == object);
-- --> true
--
-- @param value Any value.
-- @return Returns value.
_.identity = function(...) return ... end

local iterCollection = function(table, desc)
    local sortedKeys = getSortedKeys(table, desc)
    local i = 0
    return function ()
        if _.lt(i, #sortedKeys) then
            i = i + 1
            local key = sortedKeys[i]
            return key, table[key]
        end
    end
end

_.iter = function(value)
    if _.isString(value) then
        local i = 0
        return function()            
            if _.lt(i, #value) then
                i = i + 1
                local c = value:sub(i, i)                
                return i, c
            end
        end
    elseif _.isTable(value) then
        return iterCollection(value)
    else
        return function() end
    end
end

_.iterRight = function(value)
    if _.isString(value) then
        local i = #value + 1
        return function()            
            if _.gt(i, 1) then
                i = i - 1
                local c = value:sub(i, i)                
                return i, c
            end
        end
    elseif _.isTable(value) then
        return iterCollection(value, true)
    else
        return function() end
    end
end

---
-- A no-operation function that returns nil regardless of the arguments
-- it receives.
-- @usage local object = {x=5}
-- _.print(_.noop(object) == nil);
-- --> true
--
-- @param ... Any arguments
-- @return Returns nil
_.noop = function(...) return nil end


---
-- Print more readable representation of arguments using _.str 
-- @usage _.print({1, 2, 3, 4, {k=2, {'x', 'y'}}})
-- --> {1, 2, 3, 4, {{"x", "y"}, [k]=2}}
--
-- @param ... values to print
-- @return Return human readable string of the value
_.print = function(...)
    local t = _.table(...)
    for i, v in ipairs(t) do 
        t[i] = _.str(t[i])
    end
    return print(_.args(t))
end
    
---
-- Creates an array of numbers (positive and/or negative) progressing 
-- from start up to, including, end. 
-- If end is not specified it’s set to start with start then set to 1. 
-- @usage _.print(_.range(5, 20, 3))
-- --> {5, 8, 11, 14, 17, 20}
--
-- @param[opt=1] start The start of the range.
-- @param stop Then end of the range.
-- @param[opt=1] step The value to increment or decrement by
-- @return Returns the new array of numbers
_.range = function(start, ...)
    local start = start 
    local args = _.table(...)
    local a, b, c
    if #args == 0 then
        a = 1   -- according to lua 
        b = start
        c = 1
    else
        a = start
        b = args[1] 
        c = args[2] or 1
    end
    local t = {}    
    for i = a, b, c do
        _.push(t, i)
    end
    return t
end

return _

