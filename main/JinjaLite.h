#pragma once

#include <json/json.h>
#include <map>
#include <string>

// A small, safe subset of the Jinja2 template language, as used by Home Assistant
// MQTT discovery payloads (for example Zigbee2MQTT).
//
// Supported:
//   - text, {{ expression }}, {# comment #}, whitespace control ({%- -%}, {{- -}})
//   - {% set name = expression %}
//   - {% if %} / {% elif %} / {% else %} / {% endif %} (nestable)
//   - literals: 'str', "str", numbers, true/false/True/False, none/None, [list], {dict}
//   - variables, attribute access a.b, subscripts a["b"] / a[var] / a[0]
//   - == != < > <= >=, and, or, not, in, not in, is [not] defined/undefined/none/string/number/mapping
//   - inline "x if cond else y", parentheses, + - * / // % ~
//   - methods: keys() values() get() lower() upper() strip() split() startswith() endswith() replace()
//   - filters: float int string lower upper round default/d tojson/to_json length abs trim bool
//   - functions: float int string bool abs round length
//
// Anything else (loops, macros, includes, unknown filters, ...) makes Render fail.
// Missing keys evaluate to undefined and render as an empty string, None renders as an empty string.
// Render never throws and its work is bounded (template size, expression and value nesting depth, evaluation
// steps, container sizes, total memory created per render, output size).
// Parsed templates are cached (thread safe), so rendering the same template repeatedly is cheap.
namespace JinjaLite
{
	typedef std::map<std::string, Json::Value> Variables;

	// Non-owning variant, the pointed to values must outlive the Render call. Avoids copying large payloads.
	typedef std::map<std::string, const Json::Value*> VariableRefs;

	// Returns true when the template contains statement blocks ({% ... %})
	bool HasStatements(const std::string& szTemplate);

	// Renders szTemplate with the given variables. On failure false is returned, szOutput is cleared
	// and (optionally) pError receives a short reason.
	bool Render(const std::string& szTemplate, const Variables& vars, std::string& szOutput, std::string* pError = nullptr);
	bool Render(const std::string& szTemplate, const VariableRefs& vars, std::string& szOutput, std::string* pError = nullptr);
} // namespace JinjaLite
