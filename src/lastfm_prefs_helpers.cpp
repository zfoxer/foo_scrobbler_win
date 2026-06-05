//
//  lastfm_prefs_helpers.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "lastfm_prefs_helpers.h"

#include <cctype>
#include <vector>

namespace lastfm::preferences
{

std::string trimCopy(const std::string& in)
{
    std::size_t b = 0;
    while (b < in.size() && std::isspace(static_cast<unsigned char>(in[b])))
        ++b;

    std::size_t e = in.size();
    while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1])))
        --e;

    return (e > b) ? in.substr(b, e - b) : std::string{};
}

std::string lowerCopy(const std::string& in)
{
    std::string out;
    out.reserve(in.size());
    for (unsigned char c : in)
        out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

std::string titleFormatLiteral(const std::string& value)
{
    std::string out = "'";
    for (char c : value)
    {
        if (c == '\'')
            out += "''";
        else
            out.push_back(c);
    }
    out += "'";
    return out;
}

std::string makeTemplateExpression(const char* field, bool contains, const std::string& rawValues)
{
    std::vector<std::string> conditions;
    std::size_t start = 0;
    while (start <= rawValues.size())
    {
        std::size_t end = rawValues.find(';', start);
        if (end == std::string::npos)
            end = rawValues.size();

        const std::string value = trimCopy(rawValues.substr(start, end - start));
        if (!value.empty())
        {
            conditions.push_back(contains ? "$strstr($lower(" + std::string(field) + ")," +
                                                titleFormatLiteral(lowerCopy(value)) + ")"
                                          : "$stricmp(" + std::string(field) + "," + titleFormatLiteral(value) + ")");
        }

        start = end + 1;
    }

    if (conditions.empty())
        return {};

    std::string predicate = conditions.front();
    if (conditions.size() > 1)
    {
        predicate = "$or(";
        for (std::size_t i = 0; i < conditions.size(); ++i)
            predicate += (i ? "," : "") + conditions[i];
        predicate += ")";
    }

    return "$if(" + predicate + ",1,)";
}

std::string removeTemplateExpression(std::string text, const std::string& expr)
{
    if (expr.empty())
        return text;

    for (std::size_t pos = text.find(expr); pos != std::string::npos; pos = text.find(expr, pos))
    {
        text.erase(pos, expr.size());
        if (pos < text.size() && text[pos] == ' ')
            text.erase(pos, 1);
        else if (pos > 0 && text[pos - 1] == ' ')
            text.erase(pos - 1, 1);
    }
    return trimCopy(text);
}

bool hasTemplateExpression(const std::string& text, const std::string& expr)
{
    return !expr.empty() && text.find(expr) != std::string::npos;
}

std::string appendTemplateExpression(std::string text, const std::string& expr)
{
    if (expr.empty() || hasTemplateExpression(text, expr))
        return text;
    text = trimCopy(text);
    text += text.empty() ? expr : " " + expr;
    return text;
}

} // namespace lastfm::preferences
