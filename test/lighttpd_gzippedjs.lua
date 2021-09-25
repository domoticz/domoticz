if (string.match(lighty.env["physical.path"],".js.gz")) then
	lighty.header["Content-Type"] = "text/javascript"
	lighty.header["Content-Encoding"] = "gzip"
end
