#pragma once

namespace domoticz {
	class noncopyable {
	public:
		noncopyable() = default;
		~noncopyable() = default;
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
	};
} //end namespace domoticz

