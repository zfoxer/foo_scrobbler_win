//
//  lastfm_prefs_helpers.h
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#pragma once

#include <string>

namespace lastfm::preferences
{

std::string trimCopy(const std::string& in);
std::string lowerCopy(const std::string& in);
std::string titleFormatLiteral(const std::string& value);
std::string makeTemplateExpression(const char* field, bool contains, const std::string& rawValues);
std::string removeTemplateExpression(std::string text, const std::string& expr);
bool hasTemplateExpression(const std::string& text, const std::string& expr);
std::string appendTemplateExpression(std::string text, const std::string& expr);

} // namespace lastfm::preferences
