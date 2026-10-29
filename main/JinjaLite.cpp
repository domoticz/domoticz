#include "stdafx.h"
#include "JinjaLite.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace JinjaLite
{
	namespace
	{
		const size_t kMaxTemplateSize = 65536;
		const size_t kMaxOutputSize = 65536;
		const int kMaxDepth = 48;
		const int kMaxNodeDepth = 64;	 // depth of an expression tree
		const int kMaxValueDepth = 32;	 // nesting of lists/dicts built by a template
		const int kMaxSteps = 50000;
		const int kMaxReprDepth = 32;
		const size_t kMaxContainer = 4096; // elements of a list/dict built by a template
		const size_t kElementCost = 32;
		const long long kBudgetUnits = 8LL * 1024 * 1024; // memory a single render may create: elements cost kElementCost, strings their length
		const size_t kMaxCachedTemplates = 32;

		// Remaining memory budget of the Render running on this thread
		thread_local long long t_budget = 0;

		// Dictionary key used for a None key, like {None: 'x'}
		const std::string kNoneKey = std::string("\x01") + "None";

		struct JinjaError
		{
			std::string msg;
		};

		[[noreturn]] void Fail(const std::string& msg)
		{
			throw JinjaError{ msg };
		}

		// A value. A null pointer is "undefined", a pointer to a null Json::Value is "None".
		// Sub values alias their parent, so reading a key from a large payload does not copy it.
		typedef std::shared_ptr<const Json::Value> Val;

		void Charge(size_t n)
		{
			if (n > static_cast<size_t>(t_budget))
				Fail("template creates too much data");
			t_budget -= static_cast<long long>(n);
		}

		std::string_view StrOf(const Json::Value& v)
		{
			const char* b = nullptr;
			const char* e = nullptr;
			if (!v.getString(&b, &e))
				return std::string_view();
			return std::string_view(b, static_cast<size_t>(e - b));
		}

		// Adds the cost of a value (deep) to cost, fails when nested too deep or when the budget can not cover it
		void DeepCost(const Json::Value& v, int depth, size_t& cost)
		{
			if (depth > kMaxValueDepth)
				Fail("value nested too deep");
			cost += kElementCost;
			if (v.isString())
				cost += StrOf(v).size();
			else if (v.isArray())
			{
				for (Json::ArrayIndex i = 0; i < v.size(); i++)
					DeepCost(v[i], depth + 1, cost);
			}
			else if (v.isObject())
			{
				for (Json::ValueConstIterator it = v.begin(); it != v.end(); ++it)
					DeepCost(*it, depth + 1, cost);
			}
			if (cost > static_cast<size_t>(t_budget))
				Fail("template creates too much data");
		}

		Val MakeVal(Json::Value v)
		{
			if (v.isString())
				Charge(StrOf(v).size());
			return std::make_shared<const Json::Value>(std::move(v));
		}

		// Container that is about to be created from a copy of other values: charges the budget for it
		void ChargeContainer(const Json::Value& v)
		{
			size_t cost = 0;
			DeepCost(v, 1, cost);
			Charge(cost);
		}

		bool GetInt64(const Json::Value& v, Json::Int64& out)
		{
			if (v.type() == Json::intValue)
			{
				out = v.asInt64();
				return true;
			}
			if ((v.type() == Json::uintValue) && (v.asUInt64() <= static_cast<Json::UInt64>(std::numeric_limits<Json::Int64>::max())))
			{
				out = static_cast<Json::Int64>(v.asUInt64());
				return true;
			}
			return false;
		}

		bool IsNone(const Val& v)
		{
			return (!v) || v->isNull();
		}

		bool IsIntType(const Json::Value& v)
		{
			return (v.type() == Json::intValue) || (v.type() == Json::uintValue);
		}

		bool IsNumber(const Val& v)
		{
			return v && (IsIntType(*v) || (v->type() == Json::realValue));
		}

		bool IsString(const Val& v)
		{
			return v && v->isString();
		}

		bool Truthy(const Val& v)
		{
			if (IsNone(v))
				return false;
			switch (v->type())
			{
			case Json::booleanValue:
				return v->asBool();
			case Json::intValue:
			case Json::uintValue:
			case Json::realValue:
				return v->asDouble() != 0.0;
			case Json::stringValue:
				return !StrOf(*v).empty();
			case Json::arrayValue:
			case Json::objectValue:
				return !v->empty();
			default:
				return false;
			}
		}

		std::string FormatDouble(double d)
		{
			if (std::isnan(d))
				return "nan";
			if (std::isinf(d))
				return (d < 0) ? "-inf" : "inf";
			char buf[64];
			for (int precision = 15; precision <= 17; precision++)
			{
				snprintf(buf, sizeof(buf), "%.*g", precision, d);
				if (strtod(buf, nullptr) == d)
					break;
			}
			std::string s(buf);
			if (s.find_first_of(".en") == std::string::npos)
				s += ".0";
			return s;
		}

		std::string Repr(const Val& v, int depth);

		std::string ReprKey(const std::string& key)
		{
			if (key == kNoneKey)
				return "None";
			Val k = MakeVal(Json::Value(key));
			return Repr(k, 0);
		}

		// Python style representation, used when a list/dict/string is rendered or nested
		std::string Repr(const Val& v, int depth)
		{
			if (depth > kMaxReprDepth)
				Fail("value nested too deep");
			if (IsNone(v))
				return "None";
			switch (v->type())
			{
			case Json::booleanValue:
				return v->asBool() ? "True" : "False";
			case Json::intValue:
				return std::to_string(v->asLargestInt());
			case Json::uintValue:
				return std::to_string(v->asLargestUInt());
			case Json::realValue:
				return FormatDouble(v->asDouble());
			case Json::stringValue:
			{
				std::string s = v->asString();
				std::string out = "'";
				for (char c : s)
				{
					if ((c == '\\') || (c == '\''))
						out += '\\';
					out += c;
				}
				return out + "'";
			}
			case Json::arrayValue:
			{
				std::string out = "[";
				for (Json::ArrayIndex i = 0; i < v->size(); i++)
				{
					if (i)
						out += ", ";
					out += Repr(Val(v, &(*v)[i]), depth + 1);
				}
				return out + "]";
			}
			case Json::objectValue:
			{
				std::string out = "{";
				bool bFirst = true;
				for (const auto& name : v->getMemberNames())
				{
					if (!bFirst)
						out += ", ";
					bFirst = false;
					const Json::Value* pChild = v->find(name.data(), name.data() + name.size());
					out += ReprKey(name) + ": " + Repr(Val(v, pChild), depth + 1);
				}
				return out + "}";
			}
			default:
				return "";
			}
		}

		// What {{ expr }} writes
		std::string Stringify(const Val& v)
		{
			if (IsNone(v))
				return "";
			if (v->isString())
				return v->asString();
			return Repr(v, 0);
		}

		// String form used by the string filter / ~ operator, None is written as "None" there
		std::string StringifyForString(const Val& v)
		{
			if (!v)
				return "";
			if (v->isNull())
				return "None";
			return Stringify(v);
		}

		std::string KeyString(const Val& v)
		{
			if (IsNone(v))
				return kNoneKey;
			switch (v->type())
			{
			case Json::stringValue:
				return v->asString();
			case Json::booleanValue:
			case Json::intValue:
			case Json::uintValue:
			case Json::realValue:
				return Repr(v, 0);
			default:
				Fail("unhashable key");
			}
		}

		bool IsNumberOrBool(const Val& v)
		{
			return v && (IsNumber(v) || v->isBool());
		}

		bool Equal(const Val& a, const Val& b)
		{
			bool bNoneA = IsNone(a);
			bool bNoneB = IsNone(b);
			if (bNoneA || bNoneB)
				return bNoneA && bNoneB;
			if (IsNumberOrBool(a) && IsNumberOrBool(b))
				return a->asDouble() == b->asDouble();
			if (a->isString() && b->isString())
				return StrOf(*a) == StrOf(*b);
			if (a->type() != b->type())
				return false;
			return *a == *b;
		}

		// returns <0, 0, >0
		int Compare(const Val& a, const Val& b)
		{
			if (IsNumberOrBool(a) && IsNumberOrBool(b))
			{
				double da = a->asDouble();
				double db = b->asDouble();
				return (da < db) ? -1 : ((da > db) ? 1 : 0);
			}
			if (IsString(a) && IsString(b))
			{
				int c = StrOf(*a).compare(StrOf(*b));
				return (c < 0) ? -1 : ((c > 0) ? 1 : 0);
			}
			Fail("values can not be compared");
		}

		// ---------------------------------------------------------------------------------
		// Expression tokens
		// ---------------------------------------------------------------------------------

		enum class TK
		{
			Num,
			Str,
			Name,
			Op,
			End
		};

		struct Token
		{
			TK type = TK::End;
			std::string s;
			Json::Value num;
		};

		bool IsNameStart(char c)
		{
			return (c == '_') || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'));
		}

		bool IsDigit(char c)
		{
			return (c >= '0') && (c <= '9');
		}

		std::vector<Token> Lex(const std::string& s)
		{
			std::vector<Token> tokens;
			size_t i = 0;
			const size_t n = s.size();
			while (i < n)
			{
				char c = s[i];
				if ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
				{
					i++;
					continue;
				}
				Token t;
				if (IsDigit(c))
				{
					size_t start = i;
					bool bReal = false;
					while ((i < n) && IsDigit(s[i]))
						i++;
					if ((i + 1 < n) && (s[i] == '.') && IsDigit(s[i + 1]))
					{
						bReal = true;
						i++;
						while ((i < n) && IsDigit(s[i]))
							i++;
					}
					if ((i < n) && ((s[i] == 'e') || (s[i] == 'E')))
					{
						size_t j = i + 1;
						if ((j < n) && ((s[j] == '+') || (s[j] == '-')))
							j++;
						if ((j < n) && IsDigit(s[j]))
						{
							bReal = true;
							while ((j < n) && IsDigit(s[j]))
								j++;
							i = j;
						}
					}
					std::string num = s.substr(start, i - start);
					t.type = TK::Num;
					t.s = num;
					if (bReal || num.size() > 18)
						t.num = Json::Value(strtod(num.c_str(), nullptr));
					else
						t.num = Json::Value(static_cast<Json::Int64>(strtoll(num.c_str(), nullptr, 10)));
						if (t.num.isDouble() && (std::isnan(t.num.asDouble()) || std::isinf(t.num.asDouble())))
							Fail("number out of range");
				}
				else if ((c == '\'') || (c == '"'))
				{
					char quote = c;
					i++;
					std::string str;
					bool bClosed = false;
					while (i < n)
					{
						char d = s[i++];
						if (d == quote)
						{
							bClosed = true;
							break;
						}
						if (d == '\\')
						{
							if (i >= n)
								break;
							char e = s[i++];
							switch (e)
							{
							case 'n': str += '\n'; break;
							case 't': str += '\t'; break;
							case 'r': str += '\r'; break;
							default: str += e; break;
							}
						}
						else
							str += d;
					}
					if (!bClosed)
						Fail("unterminated string");
					t.type = TK::Str;
					t.s = str;
				}
				else if (IsNameStart(c))
				{
					size_t start = i;
					while ((i < n) && (IsNameStart(s[i]) || IsDigit(s[i])))
						i++;
					t.type = TK::Name;
					t.s = s.substr(start, i - start);
				}
				else
				{
					t.type = TK::Op;
					if ((i + 1 < n) && (((c == '=') && (s[i + 1] == '=')) || ((c == '!') && (s[i + 1] == '=')) || ((c == '<') && (s[i + 1] == '=')) || ((c == '>') && (s[i + 1] == '=')) || ((c == '/') && (s[i + 1] == '/'))))
					{
						t.s = s.substr(i, 2);
						i += 2;
					}
					else if (std::string("+-*/%~<>=()[]{}.,:|").find(c) != std::string::npos)
					{
						t.s = std::string(1, c);
						i++;
					}
					else
						Fail(std::string("unexpected character '") + c + "'");
				}
				tokens.push_back(std::move(t));
				if (tokens.size() > 4096)
					Fail("expression too long");
			}
			tokens.push_back(Token());
			return tokens;
		}

		// ---------------------------------------------------------------------------------
		// Expression AST and parser
		// ---------------------------------------------------------------------------------

		enum class NT
		{
			Literal,
			Name,
			Attr,
			Subscript,
			Call,
			Kwarg,
			Filter,
			Not,
			Neg,
			Pos,
			Binary,
			Compare,
			And,
			Or,
			Ternary,
			Test,
			List,
			Dict
		};

		struct Node
		{
			NT type = NT::Literal;
			std::string str;
			Val lit; // Literal, shared by every evaluation. A pointer to a null value is None
			bool flag = false;
			int depth = 1; // height of the tree below (and including) this node
			std::vector<std::unique_ptr<Node>> kids;
		};

		typedef std::unique_ptr<Node> NodePtr;

		NodePtr MakeNode(NT type, const std::string& str = std::string())
		{
			NodePtr n(new Node());
			n->type = type;
			n->str = str;
			return n;
		}

		NodePtr MakeLiteral(Json::Value v)
		{
			NodePtr n = MakeNode(NT::Literal);
			n->lit = std::make_shared<const Json::Value>(std::move(v));
			return n;
		}

		// Called when a node has all its children. Bounds the height of every tree, so evaluating
		// and destroying it can not use unbounded stack.
		NodePtr Fin(NodePtr n)
		{
			int d = 0;
			for (const auto& k : n->kids)
				d = std::max(d, k->depth);
			n->depth = d + 1;
			if (n->depth > kMaxNodeDepth)
				Fail("expression too complex");
			return n;
		}

		class Parser
		{
		  public:
			explicit Parser(std::vector<Token> tokens)
				: m_tokens(std::move(tokens))
			{
			}

			const Token& Peek(size_t ahead = 0) const
			{
				size_t idx = m_pos + ahead;
				if (idx >= m_tokens.size())
					idx = m_tokens.size() - 1;
				return m_tokens[idx];
			}
			Token Next()
			{
				Token t = Peek();
				if (m_pos < m_tokens.size() - 1)
					m_pos++;
				return t;
			}
			bool AtEnd() const
			{
				return Peek().type == TK::End;
			}
			bool IsOp(const char* op, size_t ahead = 0) const
			{
				return (Peek(ahead).type == TK::Op) && (Peek(ahead).s == op);
			}
			bool IsName(const char* name, size_t ahead = 0) const
			{
				return (Peek(ahead).type == TK::Name) && (Peek(ahead).s == name);
			}
			void ExpectOp(const char* op)
			{
				if (!IsOp(op))
					Fail(std::string("expected '") + op + "'");
				Next();
			}

			NodePtr ParseExpression()
			{
				return ParseTernary();
			}

		  private:
			struct DepthGuard
			{
				explicit DepthGuard(Parser& p)
					: m_p(p)
				{
					if (++m_p.m_depth > kMaxDepth)
						Fail("expression nested too deep");
				}
				~DepthGuard()
				{
					m_p.m_depth--;
				}
				Parser& m_p;
			};

			static bool IsKeyword(const std::string& s)
			{
				return (s == "and") || (s == "or") || (s == "not") || (s == "in") || (s == "is") || (s == "if") || (s == "else");
			}

			NodePtr ParseTernary()
			{
				DepthGuard guard(*this);
				NodePtr value = ParseOr();
				if (IsName("if"))
				{
					Next();
					NodePtr cond = ParseOr();
					NodePtr n = MakeNode(NT::Ternary);
					n->kids.push_back(std::move(cond));
					n->kids.push_back(std::move(value));
					if (IsName("else"))
					{
						Next();
						n->kids.push_back(ParseTernary());
					}
					return Fin(std::move(n));
				}
				return value;
			}

			NodePtr ParseOr()
			{
				NodePtr left = ParseAnd();
				while (IsName("or"))
				{
					Next();
					NodePtr n = MakeNode(NT::Or);
					n->kids.push_back(std::move(left));
					n->kids.push_back(ParseAnd());
					left = Fin(std::move(n));
				}
				return left;
			}

			NodePtr ParseAnd()
			{
				NodePtr left = ParseNot();
				while (IsName("and"))
				{
					Next();
					NodePtr n = MakeNode(NT::And);
					n->kids.push_back(std::move(left));
					n->kids.push_back(ParseNot());
					left = Fin(std::move(n));
				}
				return left;
			}

			NodePtr ParseNot()
			{
				DepthGuard guard(*this);
				if (IsName("not"))
				{
					Next();
					NodePtr n = MakeNode(NT::Not);
					n->kids.push_back(ParseNot());
					return Fin(std::move(n));
				}
				return ParseCompare();
			}

			bool PeekCompareOp(std::string& op, size_t& tokens) const
			{
				const Token& t = Peek();
				if ((t.type == TK::Op) && ((t.s == "==") || (t.s == "!=") || (t.s == "<") || (t.s == ">") || (t.s == "<=") || (t.s == ">=")))
				{
					op = t.s;
					tokens = 1;
					return true;
				}
				if (IsName("in"))
				{
					op = "in";
					tokens = 1;
					return true;
				}
				if (IsName("not") && IsName("in", 1))
				{
					op = "not in";
					tokens = 2;
					return true;
				}
				return false;
			}

			NodePtr ParseCompare()
			{
				NodePtr left = ParseAdd();
				if (IsName("is"))
				{
					Next();
					NodePtr n = MakeNode(NT::Test);
					if (IsName("not"))
					{
						Next();
						n->flag = true;
					}
					if (Peek().type != TK::Name)
						Fail("expected a test name");
					n->str = Next().s;
					if ((n->str != "defined") && (n->str != "undefined") && (n->str != "none") && (n->str != "string") && (n->str != "number") && (n->str != "mapping") && (n->str != "true") && (n->str != "false"))
						Fail("unsupported test: " + n->str);
					n->kids.push_back(std::move(left));
					return Fin(std::move(n));
				}
				std::string op;
				size_t tokens = 0;
				if (PeekCompareOp(op, tokens))
				{
					for (size_t i = 0; i < tokens; i++)
						Next();
					NodePtr n = MakeNode(NT::Compare, op);
					n->kids.push_back(std::move(left));
					n->kids.push_back(ParseAdd());
					std::string op2;
					size_t tokens2 = 0;
					if (PeekCompareOp(op2, tokens2))
						Fail("chained comparison is not supported");
					return Fin(std::move(n));
				}
				return left;
			}

			NodePtr ParseAdd()
			{
				NodePtr left = ParseMul();
				while (IsOp("+") || IsOp("-") || IsOp("~"))
				{
					std::string op = Next().s;
					NodePtr n = MakeNode(NT::Binary, op);
					n->kids.push_back(std::move(left));
					n->kids.push_back(ParseMul());
					left = Fin(std::move(n));
				}
				return left;
			}

			NodePtr ParseMul()
			{
				NodePtr left = ParseUnary();
				while (IsOp("*") || IsOp("/") || IsOp("//") || IsOp("%"))
				{
					std::string op = Next().s;
					NodePtr n = MakeNode(NT::Binary, op);
					n->kids.push_back(std::move(left));
					n->kids.push_back(ParseUnary());
					left = Fin(std::move(n));
				}
				return left;
			}

			NodePtr ParseUnary()
			{
				DepthGuard guard(*this);
				if (IsOp("-") || IsOp("+"))
				{
					bool bNeg = Next().s == "-";
					NodePtr n = MakeNode(bNeg ? NT::Neg : NT::Pos);
					n->kids.push_back(ParseUnary());
					return Fin(std::move(n));
				}
				return ParsePostfix();
			}

			void ParseArgs(Node& call)
			{
				ExpectOp("(");
				while (!IsOp(")"))
				{
					if (AtEnd())
						Fail("unterminated call");
					if ((Peek().type == TK::Name) && IsOp("=", 1))
					{
						NodePtr kw = MakeNode(NT::Kwarg, Next().s);
						Next(); // =
						kw->kids.push_back(ParseTernary());
						call.kids.push_back(Fin(std::move(kw)));
					}
					else
						call.kids.push_back(ParseTernary());
					if (IsOp(","))
						Next();
					else if (!IsOp(")"))
						Fail("expected ',' or ')'");
					if (call.kids.size() > 16)
						Fail("too many arguments");
				}
				Next(); // )
			}

			NodePtr ParsePostfix()
			{
				NodePtr node = ParsePrimary();
				while (true)
				{
					DepthGuard guard(*this);
					if (IsOp("."))
					{
						Next();
						if (Peek().type != TK::Name)
							Fail("expected attribute name");
						NodePtr attr = MakeNode(NT::Attr, Next().s);
						attr->kids.push_back(std::move(node));
						node = Fin(std::move(attr));
					}
					else if (IsOp("["))
					{
						Next();
						NodePtr sub = MakeNode(NT::Subscript);
						sub->kids.push_back(std::move(node));
						sub->kids.push_back(ParseTernary());
						ExpectOp("]");
						node = Fin(std::move(sub));
					}
					else if (IsOp("("))
					{
						if ((node->type != NT::Name) && (node->type != NT::Attr))
							Fail("object is not callable");
						NodePtr call = MakeNode(NT::Call);
						call->kids.push_back(std::move(node));
						ParseArgs(*call);
						node = Fin(std::move(call));
					}
					else if (IsOp("|"))
					{
						Next();
						if (Peek().type != TK::Name)
							Fail("expected filter name");
						NodePtr filter = MakeNode(NT::Filter, Next().s);
						filter->kids.push_back(std::move(node));
						if (IsOp("("))
							ParseArgs(*filter);
						node = Fin(std::move(filter));
					}
					else
						break;
				}
				return node;
			}

			NodePtr ParsePrimary()
			{
				const Token t = Next();
				switch (t.type)
				{
				case TK::Num:
				{
					return MakeLiteral(t.num);
				}
				case TK::Str:
				{
					return MakeLiteral(Json::Value(t.s));
				}
				case TK::Name:
				{
					if ((t.s == "true") || (t.s == "True") || (t.s == "false") || (t.s == "False"))
					{
						return MakeLiteral(Json::Value((t.s == "true") || (t.s == "True")));
					}
					if ((t.s == "none") || (t.s == "None"))
						return MakeLiteral(Json::Value()); // null
					if (IsKeyword(t.s))
						Fail("unexpected '" + t.s + "'");
					return MakeNode(NT::Name, t.s);
				}
				case TK::Op:
				{
					if (t.s == "(")
					{
						NodePtr n = ParseTernary();
						ExpectOp(")");
						return n;
					}
					if (t.s == "[")
					{
						NodePtr n = MakeNode(NT::List);
						while (!IsOp("]"))
						{
							if (AtEnd())
								Fail("unterminated list");
							n->kids.push_back(ParseTernary());
							if (IsOp(","))
								Next();
							else if (!IsOp("]"))
								Fail("expected ',' or ']'");
							if (n->kids.size() > 1024)
								Fail("list too long");
						}
						Next();
						return Fin(std::move(n));
					}
					if (t.s == "{")
					{
						NodePtr n = MakeNode(NT::Dict);
						while (!IsOp("}"))
						{
							if (AtEnd())
								Fail("unterminated dict");
							n->kids.push_back(ParseTernary());
							ExpectOp(":");
							n->kids.push_back(ParseTernary());
							if (IsOp(","))
								Next();
							else if (!IsOp("}"))
								Fail("expected ',' or '}'");
							if (n->kids.size() > 2048)
								Fail("dict too long");
						}
						Next();
						return Fin(std::move(n));
					}
					Fail("unexpected '" + t.s + "'");
				}
				default:
					Fail("unexpected end of expression");
				}
			}

			std::vector<Token> m_tokens;
			size_t m_pos = 0;
			int m_depth = 0;
		};

		// ---------------------------------------------------------------------------------
		// Evaluation
		// ---------------------------------------------------------------------------------

		Val Bool(bool b)
		{
			return MakeVal(Json::Value(b));
		}

		Val Int(Json::Int64 i)
		{
			return MakeVal(Json::Value(i));
		}

		Val Dbl(double d)
		{
			if (std::isnan(d) || std::isinf(d))
				Fail("number out of range");
			return MakeVal(Json::Value(d));
		}

		std::string Trim(const std::string& s)
		{
			size_t b = s.find_first_not_of(" \t\r\n");
			if (b == std::string::npos)
				return "";
			size_t e = s.find_last_not_of(" \t\r\n");
			return s.substr(b, e - b + 1);
		}

		std::string ToLower(std::string s)
		{
			for (auto& c : s)
				if ((c >= 'A') && (c <= 'Z'))
					c = static_cast<char>(c - 'A' + 'a');
			return s;
		}

		std::string ToUpper(std::string s)
		{
			for (auto& c : s)
				if ((c >= 'a') && (c <= 'z'))
					c = static_cast<char>(c - 'a' + 'A');
			return s;
		}

		bool ParseNumber(const std::string& str, double& d)
		{
			std::string s = Trim(str);
			if (s.empty())
				return false;
			char* end = nullptr;
			d = strtod(s.c_str(), &end);
			return (end != nullptr) && (*end == '\0') && !std::isnan(d) && !std::isinf(d);
		}

		struct Args
		{
			std::vector<Val> pos;
			std::map<std::string, Val> kw;

			// Fetches an argument by position or by keyword; null pointer (undefined) if absent
			Val Get(size_t idx, const char* name) const
			{
				if (idx < pos.size())
					return pos[idx];
				auto it = kw.find(name);
				if (it != kw.end())
					return it->second;
				return nullptr;
			}
			bool Has(size_t idx, const char* name) const
			{
				return (idx < pos.size()) || (kw.find(name) != kw.end());
			}
		};

		class Evaluator
		{
		  public:
			explicit Evaluator(const VariableRefs& vars)
			{
				// Non-owning aliases, the caller keeps the values alive during the render
				for (const auto& itt : vars)
				{
					if (itt.second != nullptr)
						m_vars[itt.first] = Val(std::shared_ptr<void>(), itt.second);
				}
			}

			void SetVar(const std::string& name, const Val& v)
			{
				m_vars[name] = v;
			}

			Val Eval(const Node& n)
			{
				if (++m_steps > kMaxSteps)
					Fail("evaluation too long");
				switch (n.type)
				{
				case NT::Literal:
					return n.lit;
				case NT::Name:
				{
					auto it = m_vars.find(n.str);
					return (it == m_vars.end()) ? Val() : it->second;
				}
				case NT::Attr:
				{
					Val base = Eval(*n.kids[0]);
					return Member(base, n.str);
				}
				case NT::Subscript:
					return Subscript(Eval(*n.kids[0]), Eval(*n.kids[1]));
				case NT::Call:
					return EvalCall(n);
				case NT::Filter:
				{
					Val input = Eval(*n.kids[0]);
					Args args = EvalArgs(n, 1);
					return ApplyFilter(n.str, input, args);
				}
				case NT::Not:
					return Bool(!Truthy(Eval(*n.kids[0])));
				case NT::Neg:
				case NT::Pos:
				{
					Val v = Eval(*n.kids[0]);
					if (!IsNumber(v))
						Fail("bad operand for unary operator");
					if (n.type == NT::Pos)
						return v;
					Json::Int64 i = 0;
					if (GetInt64(*v, i) && (i != std::numeric_limits<Json::Int64>::min()))
						return Int(-i);
					return Dbl(-v->asDouble());
				}
				case NT::Binary:
					return Binary(n.str, Eval(*n.kids[0]), Eval(*n.kids[1]));
				case NT::Compare:
					return CompareOp(n.str, Eval(*n.kids[0]), Eval(*n.kids[1]));
				case NT::And:
				{
					Val l = Eval(*n.kids[0]);
					if (!Truthy(l))
						return l;
					return Eval(*n.kids[1]);
				}
				case NT::Or:
				{
					Val l = Eval(*n.kids[0]);
					if (Truthy(l))
						return l;
					return Eval(*n.kids[1]);
				}
				case NT::Ternary:
				{
					if (Truthy(Eval(*n.kids[0])))
						return Eval(*n.kids[1]);
					if (n.kids.size() > 2)
						return Eval(*n.kids[2]);
					return Val(); // undefined
				}
				case NT::Test:
					return Bool(RunTest(n.str, Eval(*n.kids[0])) != n.flag);
				case NT::List:
				{
					Json::Value arr(Json::arrayValue);
					for (const auto& k : n.kids)
					{
						Val v = Eval(*k);
						if (v)
							ChargeContainer(*v);
						else
							Charge(kElementCost);
						arr.append(v ? *v : Json::Value());
					}
					return MakeVal(std::move(arr));
				}
				case NT::Dict:
				{
					Json::Value obj(Json::objectValue);
					for (size_t i = 0; i + 1 < n.kids.size(); i += 2)
					{
						std::string key = KeyString(Eval(*n.kids[i]));
						Val v = Eval(*n.kids[i + 1]);
						Charge(kElementCost + key.size());
						if (v)
							ChargeContainer(*v);
						else
							Charge(kElementCost);
						obj[key] = v ? *v : Json::Value();
					}
					return MakeVal(std::move(obj));
				}
				case NT::Kwarg:
					Fail("unexpected keyword argument");
				}
				Fail("unsupported expression");
			}

		  private:
			static Val Member(const Val& base, const std::string& name)
			{
				if (base && base->isObject())
				{
					const Json::Value* p = base->find(name.data(), name.data() + name.size());
					if (p != nullptr)
						return Val(base, p);
				}
				return Val();
			}

			static Val Subscript(const Val& base, const Val& idx)
			{
				if (IsNone(base))
					return Val();
				if (base->isObject())
				{
					std::string key = KeyString(idx);
					const Json::Value* p = base->find(key.data(), key.data() + key.size());
					if (p != nullptr)
						return Val(base, p);
					return Val();
				}
				if (base->isArray() || base->isString())
				{
					if (!IsNumber(idx))
						return Val();
					double d = idx->asDouble();
					if (!(std::fabs(d) < 9e15)) // also catches NaN
						return Val();
					Json::Int64 i = static_cast<Json::Int64>(d);
					const std::string_view str = base->isString() ? StrOf(*base) : std::string_view();
					Json::Int64 size = base->isArray() ? static_cast<Json::Int64>(base->size()) : static_cast<Json::Int64>(str.size());
					if (i < 0)
						i += size;
					if ((i < 0) || (i >= size))
						return Val();
					if (base->isArray())
						return Val(base, &(*base)[static_cast<Json::ArrayIndex>(i)]);
					return MakeVal(Json::Value(std::string(1, str[static_cast<size_t>(i)])));
				}
				return Val();
			}

			Args EvalArgs(const Node& n, size_t first)
			{
				Args args;
				for (size_t i = first; i < n.kids.size(); i++)
				{
					const Node& k = *n.kids[i];
					if (k.type == NT::Kwarg)
						args.kw[k.str] = Eval(*k.kids[0]);
					else
					{
						if (!args.kw.empty())
							Fail("positional argument follows keyword argument");
						args.pos.push_back(Eval(k));
					}
				}
				return args;
			}

			Val EvalCall(const Node& n)
			{
				const Node& callee = *n.kids[0];
				if (callee.type == NT::Name)
				{
					Args args = EvalArgs(n, 1);
					static const char* functions[] = { "float", "int", "string", "bool", "abs", "round", "length" };
					for (const char* f : functions)
					{
						if (callee.str == f)
						{
							if (args.pos.empty())
								Fail("missing argument");
							Val input = args.pos[0];
							args.pos.erase(args.pos.begin());
							return ApplyFilter(callee.str, input, args);
						}
					}
					Fail("unknown function: " + callee.str);
				}
				// method call
				Val self = Eval(*callee.kids[0]);
				Args args = EvalArgs(n, 1);
				return CallMethod(callee.str, self, args);
			}

			static Val CallMethod(const std::string& name, const Val& self, const Args& args)
			{
				if (IsNone(self))
					Fail("method called on undefined or None");

				if (self->isObject())
				{
					if (name == "keys")
					{
						if (self->size() > kMaxContainer)
							Fail("too many elements");
						Json::Value arr(Json::arrayValue);
						for (const auto& key : self->getMemberNames())
						{
							Charge(kElementCost + key.size());
							arr.append((key == kNoneKey) ? Json::Value() : Json::Value(key));
						}
						return MakeVal(std::move(arr));
					}
					if (name == "values")
					{
						if (self->size() > kMaxContainer)
							Fail("too many elements");
						ChargeContainer(*self);
						Json::Value arr(Json::arrayValue);
						for (const auto& key : self->getMemberNames())
							arr.append((*self)[key]);
						return MakeVal(std::move(arr));
					}
					if (name == "get")
					{
						Val key = args.Get(0, "key");
						std::string k = KeyString(key);
						const Json::Value* p = self->find(k.data(), k.data() + k.size());
						if (p != nullptr)
							return Val(self, p);
						Val def = args.Get(1, "default");
						return def ? def : MakeVal(Json::Value());
					}
				}
				else if (self->isString())
				{
					const std::string s = self->asString();
					if (name == "lower")
						return MakeVal(Json::Value(ToLower(s)));
					if (name == "upper")
						return MakeVal(Json::Value(ToUpper(s)));
					if (name == "strip")
						return MakeVal(Json::Value(Trim(s)));
					if (name == "startswith" || name == "endswith")
					{
						Val a = args.Get(0, "prefix");
						if (!IsString(a))
							Fail("string expected");
						std::string p = a->asString();
						bool b = (p.size() <= s.size()) && ((name == "startswith") ? (s.compare(0, p.size(), p) == 0) : (s.compare(s.size() - p.size(), p.size(), p) == 0));
						return Bool(b);
					}
					if (name == "replace")
					{
						Val a = args.Get(0, "old");
						Val b = args.Get(1, "new");
						if (!IsString(a) || !IsString(b) || a->asString().empty())
							Fail("string expected");
						std::string from = a->asString();
						std::string to = b->asString();
						std::string out;
						size_t pos = 0;
						while (true)
						{
							size_t f = s.find(from, pos);
							if (f == std::string::npos)
							{
								out += s.substr(pos);
								break;
							}
							out += s.substr(pos, f - pos) + to;
							pos = f + from.size();
							if (out.size() > kMaxOutputSize)
								Fail("result too large");
						}
						return MakeVal(Json::Value(out));
					}
					if (name == "split")
					{
						Val a = args.Get(0, "sep");
						Json::Value arr(Json::arrayValue);
						Charge(s.size());
						auto AppendPiece = [](Json::Value& list, std::string piece)
						{
							if (list.size() >= kMaxContainer)
								Fail("too many elements");
							Charge(kElementCost);
							list.append(std::move(piece));
						};
						if (IsNone(a))
						{
							size_t pos = 0;
							while (true)
							{
								size_t b = s.find_first_not_of(" \t\r\n", pos);
								if (b == std::string::npos)
									break;
								size_t e = s.find_first_of(" \t\r\n", b);
								if (e == std::string::npos)
									e = s.size();
								AppendPiece(arr, s.substr(b, e - b));
								pos = e;
							}
						}
						else
						{
							if (!IsString(a) || a->asString().empty())
								Fail("string expected");
							std::string sep = a->asString();
							size_t pos = 0;
							while (true)
							{
								size_t f = s.find(sep, pos);
								if (f == std::string::npos)
								{
									AppendPiece(arr, s.substr(pos));
									break;
								}
								AppendPiece(arr, s.substr(pos, f - pos));
								pos = f + sep.size();
							}
						}
						return MakeVal(std::move(arr));
					}
				}
				Fail("unsupported method: " + name);
			}

			static Val ToFloat(const Val& v, const Args& args)
			{
				double d = 0;
				if (v && v->isBool())
					return Dbl(v->asBool() ? 1.0 : 0.0);
				if (IsNumber(v))
					return Dbl(v->asDouble());
				if (IsString(v) && ParseNumber(v->asString(), d))
					return Dbl(d);
				if (args.Has(0, "default"))
					return args.Get(0, "default");
				Fail("value can not be converted to float");
			}

			static Val ToInt(const Val& v, const Args& args)
			{
				double d = 0;
				if (v && v->isBool())
					return Int(v->asBool() ? 1 : 0);
				if (IsNumber(v))
				{
					if (IsIntType(*v))
						return v;
					d = v->asDouble();
				}
				else if (IsString(v) && ParseNumber(v->asString(), d))
				{
				}
				else if (args.Has(0, "default"))
					return args.Get(0, "default");
				else
					Fail("value can not be converted to int");
				if (!(std::fabs(d) <= 9e15))
					Fail("number out of range");
				return Int(static_cast<Json::Int64>(d)); // truncates like python int()
			}

			static Val ApplyFilter(const std::string& name, const Val& v, const Args& args)
			{
				if (name == "float")
					return ToFloat(v, args);
				if (name == "int")
					return ToInt(v, args);
				if (name == "string")
					return MakeVal(Json::Value(StringifyForString(v)));
				if (name == "lower")
					return MakeVal(Json::Value(ToLower(StringifyForString(v))));
				if (name == "upper")
					return MakeVal(Json::Value(ToUpper(StringifyForString(v))));
				if ((name == "trim") || (name == "strip"))
					return MakeVal(Json::Value(Trim(StringifyForString(v))));
				if ((name == "default") || (name == "d"))
				{
					Val def = args.Has(0, "default_value") ? args.Get(0, "default_value") : MakeVal(Json::Value(std::string()));
					bool bBoolean = args.Has(1, "boolean") && Truthy(args.Get(1, "boolean"));
					if (!v)
						return def;
					if (bBoolean && !Truthy(v))
						return def;
					return v;
				}
				if (name == "bool")
				{
					if (IsString(v))
					{
						std::string s = ToLower(Trim(v->asString()));
						return Bool((s == "true") || (s == "yes") || (s == "on") || (s == "1") || (s == "enable"));
					}
					return Bool(Truthy(v));
				}
				if (name == "abs")
				{
					if (!IsNumber(v))
						Fail("abs needs a number");
					Json::Int64 i = 0;
					if (GetInt64(*v, i) && (i != std::numeric_limits<Json::Int64>::min()))
						return Int(i < 0 ? -i : i);
					return Dbl(std::fabs(v->asDouble()));
				}
				if (name == "round")
				{
					if (!IsNumber(v))
						Fail("round needs a number");
					Val p = args.Get(0, "precision");
					int precision = 0;
					if (!IsNone(p))
					{
						if (!IsNumber(p))
							Fail("precision must be a number");
						double dp = p->asDouble();
						if (!(std::fabs(dp) <= 15.0)) // also catches NaN
							Fail("precision out of range");
						precision = static_cast<int>(dp);
					}
					Val m = args.Get(1, "method");
					std::string method = IsString(m) ? m->asString() : "common";
					double scale = std::pow(10.0, precision);
					double x = v->asDouble() * scale;
					if (method == "floor")
						x = std::floor(x);
					else if (method == "ceil")
						x = std::ceil(x);
					else if (method == "common")
						x = std::nearbyint(x);
					else
						Fail("unsupported round method");
					return Dbl(x / scale);
				}
				if ((name == "tojson") || (name == "to_json"))
				{
					Json::StreamWriterBuilder builder;
					builder["indentation"] = "";
					builder["precision"] = 15;
					builder["emitUTF8"] = true;
					return MakeVal(Json::Value(Json::writeString(builder, v ? *v : Json::Value())));
				}
				if ((name == "length") || (name == "count"))
				{
					if (!v)
						Fail("length of undefined");
					if (v->isString())
						return Int(static_cast<Json::Int64>(StrOf(*v).size()));
					if (v->isArray() || v->isObject())
						return Int(static_cast<Json::Int64>(v->size()));
					Fail("object has no length");
				}
				Fail("unsupported filter: " + name);
			}

			static bool RunTest(const std::string& test, const Val& v)
			{
				if (test == "defined")
					return static_cast<bool>(v);
				if (test == "undefined")
					return !v;
				if (test == "none")
					return v && v->isNull();
				if (test == "string")
					return IsString(v);
				if (test == "number")
					return IsNumber(v);
				if (test == "mapping")
					return v && v->isObject();
				if (test == "true")
					return v && v->isBool() && v->asBool();
				if (test == "false")
					return v && v->isBool() && !v->asBool();
				Fail("unsupported test");
			}

			static Val CompareOp(const std::string& op, const Val& a, const Val& b)
			{
				if (op == "==")
					return Bool(Equal(a, b));
				if (op == "!=")
					return Bool(!Equal(a, b));
				if ((op == "in") || (op == "not in"))
				{
					bool bFound = false;
					if (!IsNone(b))
					{
						if (b->isObject())
						{
							std::string key = KeyString(a);
							bFound = b->find(key.data(), key.data() + key.size()) != nullptr;
						}
						else if (b->isArray())
						{
							for (Json::ArrayIndex i = 0; i < b->size(); i++)
							{
								if (Equal(a, Val(b, &(*b)[i])))
								{
									bFound = true;
									break;
								}
							}
						}
						else if (b->isString())
						{
							if (!IsString(a))
								Fail("'in <string>' needs a string");
							bFound = StrOf(*b).find(StrOf(*a)) != std::string_view::npos;
						}
						else
							Fail("argument is not iterable");
					}
					return Bool(bFound != (op == "not in"));
				}
				int c = Compare(a, b);
				if (op == "<")
					return Bool(c < 0);
				if (op == ">")
					return Bool(c > 0);
				if (op == "<=")
					return Bool(c <= 0);
				if (op == ">=")
					return Bool(c >= 0);
				Fail("unsupported operator");
			}

			static Val Binary(const std::string& op, const Val& a, const Val& b)
			{
				if (op == "~")
				{
					std::string s = StringifyForString(a);
					s += StringifyForString(b);
					if (s.size() > kMaxOutputSize)
						Fail("result too large");
					return MakeVal(Json::Value(s));
				}
				if ((op == "+") && IsString(a) && IsString(b))
				{
					std::string_view sa = StrOf(*a);
					std::string_view sb = StrOf(*b);
					if (sa.size() + sb.size() > kMaxOutputSize)
						Fail("result too large");
					std::string s;
					s.reserve(sa.size() + sb.size());
					s.append(sa).append(sb);
					return MakeVal(Json::Value(s));
				}
				if ((op == "+") && a && b && a->isArray() && b->isArray())
				{
					if (a->size() + b->size() > kMaxContainer)
						Fail("too many elements");
					ChargeContainer(*a);
					ChargeContainer(*b);
					Json::Value arr(*a);
					for (Json::ArrayIndex i = 0; i < b->size(); i++)
						arr.append((*b)[i]);
					return MakeVal(std::move(arr));
				}
				if (!IsNumber(a) || !IsNumber(b))
					Fail("unsupported operand types");

				const bool bInts = IsIntType(*a) && IsIntType(*b);
				const double da = a->asDouble();
				const double db = b->asDouble();
				double r;
				if (op == "+")
					r = da + db;
				else if (op == "-")
					r = da - db;
				else if (op == "*")
					r = da * db;
				else if (op == "/")
				{
					if (db == 0.0)
						Fail("division by zero");
					return Dbl(da / db);
				}
				else if (op == "//")
				{
					if (db == 0.0)
						Fail("division by zero");
					r = std::floor(da / db);
				}
				else if (op == "%")
				{
					if (db == 0.0)
						Fail("division by zero");
					r = da - db * std::floor(da / db);
				}
				else
					Fail("unsupported operator");
				if (bInts && (std::fabs(r) < 9e15))
					return Int(static_cast<Json::Int64>(r));
				return Dbl(r);
			}

			std::map<std::string, Val> m_vars;
			int m_steps = 0;
		};

		// ---------------------------------------------------------------------------------
		// Template structure
		// ---------------------------------------------------------------------------------

		struct Seg
		{
			enum Kind
			{
				Text,
				Output,
				Statement
			};
			Kind kind = Text;
			std::string s;
		};

		struct Stmt;

		struct Branch
		{
			NodePtr cond; // null for else
			std::vector<std::unique_ptr<Stmt>> body;
		};

		struct Stmt
		{
			enum Kind
			{
				Text,
				Output,
				Set,
				If
			};
			Kind kind = Text;
			std::string text; // Text, or the variable name for Set
			NodePtr expr;	  // Output, Set
			std::vector<Branch> branches;
		};

		// Finds the end of a {{ }} / {% %} block, skipping string literals and nested braces. Returns npos if not found.
		size_t FindBlockEnd(const std::string& s, size_t start, const char* end)
		{
			const size_t endLen = 2;
			int depth = 0;
			size_t i = start;
			while (i < s.size())
			{
				char c = s[i];
				if ((c == '\'') || (c == '"'))
				{
					char quote = c;
					i++;
					while ((i < s.size()) && (s[i] != quote))
					{
						if (s[i] == '\\')
							i++;
						i++;
					}
					i++;
					continue;
				}
				if ((depth == 0) && (s.compare(i, endLen, end) == 0))
					return i;
				if (c == '{')
					depth++;
				else if ((c == '}') && (depth > 0))
					depth--;
				i++;
			}
			return std::string::npos;
		}

		std::vector<Seg> Split(const std::string& tmpl)
		{
			std::vector<Seg> segs;
			size_t pos = 0;
			bool bTrimNext = false;
			auto addText = [&](std::string text)
			{
				if (bTrimNext)
				{
					size_t b = text.find_first_not_of(" \t\r\n");
					text = (b == std::string::npos) ? std::string() : text.substr(b);
					bTrimNext = false;
				}
				if (!text.empty())
				{
					Seg s;
					s.kind = Seg::Text;
					s.s = std::move(text);
					segs.push_back(std::move(s));
				}
			};
			while (pos < tmpl.size())
			{
				// One linear scan for the next opener
				size_t open = pos;
				while (true)
				{
					open = tmpl.find('{', open);
					if (open == std::string::npos)
						break;
					if ((open + 1 < tmpl.size()) && ((tmpl[open + 1] == '{') || (tmpl[open + 1] == '%') || (tmpl[open + 1] == '#')))
						break;
					open++;
				}
				if (open == std::string::npos)
				{
					addText(tmpl.substr(pos));
					break;
				}
				const char kindCh = tmpl[open + 1];
				std::string text = tmpl.substr(pos, open - pos);
				size_t contentStart = open + 2;
				bool bTrimPrev = false;
				if ((kindCh != '#') && (contentStart < tmpl.size()) && (tmpl[contentStart] == '-'))
				{
					bTrimPrev = true;
					contentStart++;
				}
				if (bTrimPrev)
				{
					size_t e = text.find_last_not_of(" \t\r\n");
					text = (e == std::string::npos) ? std::string() : text.substr(0, e + 1);
				}
				addText(text);

				const char* endMarker = (kindCh == '{') ? "}}" : ((kindCh == '%') ? "%}" : "#}");
				size_t end = (kindCh == '#') ? tmpl.find("#}", contentStart) : FindBlockEnd(tmpl, contentStart, endMarker);
				if (end == std::string::npos)
					Fail("unterminated block");
				std::string content = tmpl.substr(contentStart, end - contentStart);
				if ((kindCh != '#') && !content.empty() && (content.back() == '-'))
				{
					content.pop_back();
					bTrimNext = true;
				}
				pos = end + 2;
				if (kindCh == '#')
					continue;
				Seg s;
				s.kind = (kindCh == '{') ? Seg::Output : Seg::Statement;
				s.s = std::move(content);
				segs.push_back(std::move(s));
				if (segs.size() > 8192)
					Fail("template too complex");
			}
			return segs;
		}

		class TemplateParser
		{
		  public:
			explicit TemplateParser(const std::vector<Seg>& segs)
				: m_segs(segs)
			{
			}

			std::vector<std::unique_ptr<Stmt>> ParseAll()
			{
				std::string terminator;
				std::vector<std::unique_ptr<Stmt>> body = ParseBlock(0, terminator);
				if (!terminator.empty())
					Fail("unexpected {% " + terminator + " %}");
				return body;
			}

		  private:
			std::vector<std::unique_ptr<Stmt>> ParseBlock(int depth, std::string& terminator)
			{
				if (depth > kMaxDepth)
					Fail("blocks nested too deep");
				std::vector<std::unique_ptr<Stmt>> body;
				terminator.clear();
				while (m_idx < m_segs.size())
				{
					const Seg& seg = m_segs[m_idx];
					if (seg.kind == Seg::Text)
					{
						std::unique_ptr<Stmt> st(new Stmt());
						st->kind = Stmt::Text;
						st->text = seg.s;
						body.push_back(std::move(st));
						m_idx++;
						continue;
					}
					if (seg.kind == Seg::Output)
					{
						Parser p(Lex(seg.s));
						std::unique_ptr<Stmt> st(new Stmt());
						st->kind = Stmt::Output;
						st->expr = p.ParseExpression();
						if (!p.AtEnd())
							Fail("unexpected token in expression");
						body.push_back(std::move(st));
						m_idx++;
						continue;
					}

					// statement
					Parser p(Lex(seg.s));
					if (p.Peek().type != TK::Name)
						Fail("empty or invalid statement");
					const std::string kw = p.Next().s;
					if (kw == "set")
					{
						if (p.Peek().type != TK::Name)
							Fail("expected a variable name");
						std::unique_ptr<Stmt> st(new Stmt());
						st->kind = Stmt::Set;
						st->text = p.Next().s;
						p.ExpectOp("=");
						st->expr = p.ParseExpression();
						if (!p.AtEnd())
							Fail("unexpected token in set");
						body.push_back(std::move(st));
						m_idx++;
					}
					else if (kw == "if")
					{
						std::unique_ptr<Stmt> st(new Stmt());
						st->kind = Stmt::If;
						Branch br;
						br.cond = p.ParseExpression();
						if (!p.AtEnd())
							Fail("unexpected token in if");
						m_idx++;
						bool bSeenElse = false;
						while (true)
						{
							std::string term;
							br.body = ParseBlock(depth + 1, term);
							st->branches.push_back(std::move(br));
							br = Branch();
							if (m_branchCount++ > 4096)
								Fail("too many branches");
							if (term.empty())
								Fail("missing {% endif %}");
							if (term == "endif")
							{
								m_idx++;
								break;
							}
							// elif / else: m_idx is at the terminating statement
							Parser tp(Lex(m_segs[m_idx].s));
							tp.Next();
							if (bSeenElse)
								Fail("unexpected statement after else");
							if (term == "elif")
							{
								br.cond = tp.ParseExpression();
								if (!tp.AtEnd())
									Fail("unexpected token in elif");
							}
							else
							{
								if (!tp.AtEnd())
									Fail("unexpected token in else");
								bSeenElse = true;
							}
							m_idx++;
						}
						body.push_back(std::move(st));
					}
					else if ((kw == "elif") || (kw == "else") || (kw == "endif"))
					{
						terminator = kw;
						return body; // handled by the "if" parser, m_idx stays at this statement
					}
					else
						Fail("unsupported statement: " + kw);
				}
				return body;
			}

			const std::vector<Seg>& m_segs;
			size_t m_idx = 0;
			int m_branchCount = 0;
		};

		void Append(std::string& out, const std::string& s)
		{
			if (out.size() + s.size() > kMaxOutputSize)
				Fail("output too large");
			out += s;
		}

		void Execute(const std::vector<std::unique_ptr<Stmt>>& body, Evaluator& ev, std::string& out)
		{
			for (const auto& st : body)
			{
				switch (st->kind)
				{
				case Stmt::Text:
					Append(out, st->text);
					break;
				case Stmt::Output:
					Append(out, Stringify(ev.Eval(*st->expr)));
					break;
				case Stmt::Set:
					ev.SetVar(st->text, ev.Eval(*st->expr));
					break;
				case Stmt::If:
					for (const auto& br : st->branches)
					{
						if (!br.cond || Truthy(ev.Eval(*br.cond)))
						{
							Execute(br.body, ev, out);
							break;
						}
					}
					break;
				}
			}
		}
	} // namespace

	namespace
	{
		struct Compiled
		{
			std::vector<std::unique_ptr<Stmt>> body;
			std::string error;
			bool ok = false;
		};

		std::shared_ptr<const Compiled> Compile(const std::string& szTemplate)
		{
			std::shared_ptr<Compiled> c = std::make_shared<Compiled>();
			try
			{
				std::vector<Seg> segs = Split(szTemplate);
				TemplateParser parser(segs);
				c->body = parser.ParseAll();
				c->ok = true;
			}
			catch (const JinjaError& e)
			{
				c->error = e.msg;
			}
			catch (const std::exception& e)
			{
				c->error = e.what();
			}
			catch (...)
			{
				c->error = "unknown error";
			}
			if (!c->ok)
				c->body.clear();
			return c;
		}

		// Parsed templates are immutable, so they can be shared between threads
		std::mutex g_cacheMutex;
		std::unordered_map<std::string, std::shared_ptr<const Compiled>> g_cache;

		std::shared_ptr<const Compiled> GetCompiled(const std::string& szTemplate)
		{
			{
				std::lock_guard<std::mutex> lock(g_cacheMutex);
				auto it = g_cache.find(szTemplate);
				if (it != g_cache.end())
					return it->second;
			}
			std::shared_ptr<const Compiled> c = Compile(szTemplate);
			std::lock_guard<std::mutex> lock(g_cacheMutex);
			if (g_cache.size() >= kMaxCachedTemplates)
				g_cache.clear();
			g_cache[szTemplate] = c;
			return c;
		}
	} // namespace

	bool HasStatements(const std::string& szTemplate)
	{
		return szTemplate.find("{%") != std::string::npos;
	}

	bool Render(const std::string& szTemplate, const VariableRefs& vars, std::string& szOutput, std::string* pError)
	{
		szOutput.clear();
		try
		{
			if (szTemplate.size() > kMaxTemplateSize)
				Fail("template too large");
			t_budget = kBudgetUnits;
			std::shared_ptr<const Compiled> compiled = GetCompiled(szTemplate);
			if (!compiled->ok)
			{
				if (pError != nullptr)
					*pError = compiled->error;
				return false;
			}
			Evaluator ev(vars);
			std::string out;
			Execute(compiled->body, ev, out);
			szOutput = std::move(out);
			return true;
		}
		catch (const JinjaError& e)
		{
			if (pError != nullptr)
				*pError = e.msg;
		}
		catch (const std::exception& e)
		{
			if (pError != nullptr)
				*pError = e.what();
		}
		catch (...)
		{
			if (pError != nullptr)
				*pError = "unknown error";
		}
		szOutput.clear();
		return false;
	}

	bool Render(const std::string& szTemplate, const Variables& vars, std::string& szOutput, std::string* pError)
	{
		VariableRefs refs;
		for (const auto& itt : vars)
			refs[itt.first] = &itt.second;
		return Render(szTemplate, refs, szOutput, pError);
	}
} // namespace JinjaLite
