if (string.match(lighty.env["physical.path"],".json.gz")) then
	lighty.header["Content-Type"] = "application/json;charset=UTF-8"
	lighty.header["Content-Encoding"] = "gzip"
end
