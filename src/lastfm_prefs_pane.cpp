//
//  lastfm_prefs_pane.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "debug.h"
#include "lastfm_settings.h"
#include "lastfm_state.h"

#include <foobar2000/SDK/coreDarkMode.h>
#include <helpers/atl-misc.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <initializer_list>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace
{
static const GUID GUID_LASTFM_PREFERENCES_PAGE = {
    0x00b3d05b, 0x178d, 0x44bd, {0x85, 0x00, 0x48, 0x81, 0x9c, 0xba, 0x1c, 0xd6}};

constexpr int kEditRightMargin = 24;
constexpr int kTemplateCheckboxWidth = 150;
constexpr int kTemplateControlGap = 8;

enum
{
    TabScrobbling = 0,
    TabTags,
    TabExclusions,
    TabCount
};

enum ControlId
{
    IdTabs = 1000,
    IdConsoleCombo,
    IdAuthStatus,
    IdShowPlaybackMenu,
    IdDisableNowPlaying,
    IdOnlyLibrary,
    IdDynamicCombo,
    IdTreatVariousArtists,
    IdArtistTf,
    IdAlbumArtistTf,
    IdTitleTf,
    IdAlbumTf,
    IdExcludeArtists,
    IdExcludeTitles,
    IdExcludeAlbums,
    IdExcludeTf,
    IdTemplateGenre,
    IdTemplateMediaKind,
    IdTemplatePath,
    IdTemplateComment,
    IdTemplateGenreValue,
    IdTemplateMediaKindValue,
    IdTemplatePathValue,
    IdTemplateCommentValue,
};

struct TextFieldSetting
{
    int id;
    const wchar_t* label;
    std::string (*getValue)();
    void (*setValue)(const std::string&);
    const char* defaultValue;
};

const TextFieldSetting kTagFields[] = {
    {IdArtistTf, L"Artist:", lastfm::settings::artistTitleFormat, lastfm::settings::setArtistTitleFormat, "[%Artist%]"},
    {IdAlbumArtistTf, L"Album artist:", lastfm::settings::albumArtistTitleFormat,
     lastfm::settings::setAlbumArtistTitleFormat, "[%Album Artist%]"},
    {IdTitleTf, L"Title:", lastfm::settings::titleTitleFormat, lastfm::settings::setTitleTitleFormat, "[%Title%]"},
    {IdAlbumTf, L"Album:", lastfm::settings::albumTitleFormat, lastfm::settings::setAlbumTitleFormat, "[%Album%]"},
};

const TextFieldSetting kExclusionFields[] = {
    {IdExcludeArtists, L"Artists:", lastfm::settings::excludedArtistsPatternList,
     lastfm::settings::setExcludedArtistsPatternList, ""},
    {IdExcludeTitles, L"Titles:", lastfm::settings::excludedTitlesPatternList,
     lastfm::settings::setExcludedTitlesPatternList, ""},
    {IdExcludeAlbums, L"Albums:", lastfm::settings::excludedAlbumsPatternList,
     lastfm::settings::setExcludedAlbumsPatternList, ""},
    {IdExcludeTf, L"Title Formatting:", lastfm::settings::excludedTitleFormatExpression,
     lastfm::settings::setExcludedTitleFormatExpression, ""},
};

struct TemplateSetting
{
    int checkboxId;
    int editId;
    const wchar_t* label;
    const char* field;
    bool contains;
    std::string (*getValue)();
    void (*setValue)(const std::string&);
    const char* defaultValue;
};

const TemplateSetting kTemplates[] = {
    {IdTemplateGenre, IdTemplateGenreValue, L"Genre is:", "%Genre%", false,
     lastfm::settings::excludedGenreTemplateValueList, lastfm::settings::setExcludedGenreTemplateValueList, "Podcast"},
    {IdTemplateMediaKind, IdTemplateMediaKindValue, L"Media kind is:", "%Media Kind%", false,
     lastfm::settings::excludedMediaKindTemplateValueList, lastfm::settings::setExcludedMediaKindTemplateValueList,
     "Audiobook"},
    {IdTemplatePath, IdTemplatePathValue, L"Path contains:", "$if2(%Path%,) $if2(%FOO_SCROBBLER_PATH%,)", true,
     lastfm::settings::excludedPathTemplateValueList, lastfm::settings::setExcludedPathTemplateValueList,
     ".mpc; /other/"},
    {IdTemplateComment, IdTemplateCommentValue, L"Comment contains:", "%Comment%", true,
     lastfm::settings::excludedCommentTemplateValueList, lastfm::settings::setExcludedCommentTemplateValueList,
     "Interview"},
};

template <typename Callback> void forEachTextSetting(Callback callback)
{
    for (const auto& field : kTagFields)
        callback(field.id, field);
    for (const auto& field : kExclusionFields)
        callback(field.id, field);
    for (const auto& field : kTemplates)
        callback(field.editId, field);
}

std::string utf8FromWindow(HWND wnd)
{
    const int length = ::GetWindowTextLengthW(wnd);
    std::wstring buffer(static_cast<std::size_t>(length) + 1, L'\0');
    ::GetWindowTextW(wnd, buffer.data(), length + 1);
    return std::string(pfc::stringcvt::string_utf8_from_wide(buffer.c_str()).get_ptr());
}

void setWindowUtf8(HWND wnd, const std::string& value)
{
    ::SetWindowTextW(wnd, pfc::stringcvt::string_wide_from_utf8(value.c_str()).get_ptr());
}

bool checked(HWND wnd)
{
    return ::SendMessageW(wnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
void setChecked(HWND wnd, bool value)
{
    ::SendMessageW(wnd, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
}
int comboSelection(HWND wnd)
{
    return static_cast<int>(::SendMessageW(wnd, CB_GETCURSEL, 0, 0));
}
void setComboSelection(HWND wnd, int value)
{
    ::SendMessageW(wnd, CB_SETCURSEL, value, 0);
}

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
    std::string out = "\x27";
    for (char c : value)
    {
        if (static_cast<unsigned char>(c) == 0x27)
            out += "\x27\x27";
        else
            out.push_back(c);
    }
    out += "\x27";
    return out;
}

std::string makeTemplateExpression(const char* field, bool contains, const std::string& rawValues)
{
    std::vector<std::string> conditions;
    std::size_t start = 0;
    while (start <= rawValues.size())
    {
        std::size_t end = rawValues.find(0x3B, start);
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
        if (pos < text.size() && static_cast<unsigned char>(text[pos]) == 0x20)
            text.erase(pos, 1);
        else if (pos > 0 && static_cast<unsigned char>(text[pos - 1]) == 0x20)
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

std::string templateExpression(const TemplateSetting& t, const std::string& value)
{
    return makeTemplateExpression(t.field, t.contains, value);
}

class LastfmPreferencesPage : public CWindowImpl<LastfmPreferencesPage, CWindow>, public preferences_page_instance
{
  public:
    using Base = CWindowImpl<LastfmPreferencesPage, CWindow>;

    explicit LastfmPreferencesPage(preferences_page_callback::ptr callback) : callback_(callback)
    {
    }

    DECLARE_WND_CLASS_EX(L"{1C70A2BD-5B68-4D9A-9FD4-40481D5062E1}", CS_HREDRAW | CS_VREDRAW, COLOR_BTNFACE)

    HWND Create(HWND parent)
    {
        return Base::Create(parent, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            WS_EX_CONTROLPARENT);
    }

    BEGIN_MSG_MAP(LastfmPreferencesPage)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    END_MSG_MAP()

    t_uint32 get_state() override
    {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (hasChanged())
            state |= preferences_state::changed;
        return state;
    }

    void apply() override
    {
        lastfm::settings::setConsoleLevel(comboSelection(consoleCombo_));
        lastfmSetLogLevelFromConsoleChoice(lastfm::settings::consoleLevel());

        // The checkbox is forced on and disabled while logged out; only persist a state the user could edit.
        if (lastfmGetAuthState().isAuthenticated)
            lastfm::settings::setShowPlaybackMenu(checked(showPlaybackMenu_));

        lastfm::settings::setDisableNowPlaying(checked(disableNowPlaying_));
        lastfm::settings::setOnlyScrobbleFromMediaLibrary(checked(onlyLibrary_));
        lastfm::settings::setDynamicSourcesMode(comboSelection(dynamicCombo_));
        lastfm::settings::setTreatVariousArtistsAsEmpty(checked(treatVariousArtists_));

        forEachTextSetting([this](int id, const auto& setting) { setting.setValue(getText(id)); });

        refreshTemplateValueCache();
        notifyChanged();
    }

    void reset() override
    {
        loading_ = true;

        setComboSelection(consoleCombo_, lastfm::settings::ConsoleBasic);
        setChecked(showPlaybackMenu_, true);
        setChecked(disableNowPlaying_, false);
        setChecked(onlyLibrary_, false);
        setComboSelection(dynamicCombo_, lastfm::settings::DynamicSourcesNowPlayingAndScrobbling);
        setChecked(treatVariousArtists_, false);

        forEachTextSetting([this](int id, const auto& setting) { setText(id, setting.defaultValue); });

        refreshTemplateValueCache();
        refreshTemplateCheckboxes();
        refreshDynamicEnabledState();

        loading_ = false;
        notifyChanged();
    }

  private:
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&)
    {
        tabs_ = ::CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, 0, 0, 0,
                                  0, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IdTabs)),
                                  core_api::get_my_instance(), nullptr);
        dark_.AddDialog(m_hWnd);
        addDarkControl(tabs_);

        insertTab(TabScrobbling, L"Scrobbling");
        insertTab(TabTags, L"Tags");
        insertTab(TabExclusions, L"Exclusions");

        for (HWND& page : pages_)
        {
            page = ::CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 0, 0,
                                     m_hWnd, nullptr, core_api::get_my_instance(), nullptr);
            dark_.AddDialog(page);
            WIN32_OP_D(::SetWindowSubclass(page, pageSubclassProc, 0, reinterpret_cast<DWORD_PTR>(m_hWnd)));
        }

        createScrobblingPage();
        createTagsPage();
        createExclusionsPage();

        applyDialogFont();

        TabCtrl_SetCurSel(tabs_, TabScrobbling);
        showSelectedTab();

        RECT rc{};
        ::GetClientRect(m_hWnd, &rc);
        layout(rc.right - rc.left, rc.bottom - rc.top);

        loadSettings();
        refreshAuthStatus();
        ::SetTimer(m_hWnd, 1, 1000, nullptr);

        return 0;
    }

    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&)
    {
        ::KillTimer(m_hWnd, 1);
        if (boldFont_)
            ::DeleteObject(boldFont_);
        return 0;
    }

    LRESULT OnShowWindow(UINT, WPARAM wp, LPARAM, BOOL& handled)
    {
        if (wp && tabs_)
        {
            showSelectedTab();
            ::RedrawWindow(m_hWnd, nullptr, nullptr,
                           RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        handled = FALSE;
        return 0;
    }

    LRESULT OnSize(UINT, WPARAM, LPARAM lp, BOOL&)
    {
        layout(LOWORD(lp), HIWORD(lp));
        return 0;
    }

    void layout(int width, int height)
    {
        ::MoveWindow(tabs_, 0, 0, width, height, TRUE);

        RECT rc{0, 0, width, height};
        TabCtrl_AdjustRect(tabs_, FALSE, &rc);
        for (HWND page : pages_)
            ::MoveWindow(page, rc.left + 8, rc.top + 8, rc.right - rc.left - 16, rc.bottom - rc.top - 16, TRUE);

        layoutPages(rc.right - rc.left - 16);
    }

    LRESULT OnNotify(UINT, WPARAM, LPARAM lp, BOOL&)
    {
        const auto* nm = reinterpret_cast<NMHDR*>(lp);
        if (nm && nm->idFrom == IdTabs && nm->code == TCN_SELCHANGE)
            showSelectedTab();
        return 0;
    }

    LRESULT OnCommand(UINT, WPARAM wp, LPARAM lp, BOOL&)
    {
        if (loading_ || lp == 0)
            return 0;

        const int id = LOWORD(wp);
        const int code = HIWORD(wp);
        if (code != BN_CLICKED && code != CBN_SELCHANGE && code != EN_CHANGE)
            return 0;

        if (id == IdOnlyLibrary)
            refreshDynamicEnabledState();
        else if (templateIndex(id) >= 0)
            updateTemplateExpressionFromControl(id);
        else if (id == IdExcludeTf)
            refreshTemplateCheckboxes();

        notifyChanged();
        return 0;
    }

    LRESULT OnTimer(UINT, WPARAM wp, LPARAM, BOOL&)
    {
        if (wp == 1)
            refreshAuthStatus();
        return 0;
    }

    static LRESULT CALLBACK pageSubclassProc(HWND wnd, UINT message, WPARAM wp, LPARAM lp, UINT_PTR subclassId,
                                             DWORD_PTR ownerData)
    {
        if (message == WM_COMMAND || message == WM_NOTIFY)
            return ::SendMessageW(reinterpret_cast<HWND>(ownerData), message, wp, lp);

        if (message == WM_NCDESTROY)
            ::RemoveWindowSubclass(wnd, pageSubclassProc, subclassId);

        return ::DefSubclassProc(wnd, message, wp, lp);
    }

    static BOOL CALLBACK setChildFont(HWND wnd, LPARAM fontData)
    {
        ::SendMessageW(wnd, WM_SETFONT, static_cast<WPARAM>(fontData), TRUE);
        return TRUE;
    }

    void applyDialogFont()
    {
        HWND host = ::GetParent(m_hWnd);
        HFONT font = reinterpret_cast<HFONT>(::SendMessageW(host, WM_GETFONT, 0, 0));
        if (!font)
            font = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));

        ::EnumChildWindows(m_hWnd, setChildFont, reinterpret_cast<LPARAM>(font));

        LOGFONTW logFont{};
        if (::GetObjectW(font, sizeof(logFont), &logFont))
        {
            logFont.lfWeight = FW_BOLD;
            boldFont_ = ::CreateFontIndirectW(&logFont);
            for (HWND header : headers_)
                ::SendMessageW(header, WM_SETFONT, reinterpret_cast<WPARAM>(boldFont_), TRUE);
        }
    }

    HWND addDarkControl(HWND wnd)
    {
        if (wnd)
            dark_.AddCtrlAuto(wnd);
        return wnd;
    }

    HWND addLabel(HWND parent, const wchar_t* text, int x, int y, int width = 150)
    {
        return addDarkControl(::CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_RIGHT, x, y + 3, width,
                                                20, parent, nullptr, core_api::get_my_instance(), nullptr));
    }

    HWND addHeader(HWND parent, const wchar_t* text, int x, int y)
    {
        HWND header = addDarkControl(::CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, 360,
                                                       20, parent, nullptr, core_api::get_my_instance(), nullptr));
        headers_.push_back(header);
        return header;
    }

    HWND addEdit(HWND parent, int id, int x, int y)
    {
        return addDarkControl(::CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x, y, 360, 23, parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr));
    }

    HWND addCheckbox(HWND parent, int id, const wchar_t* text, int x, int y, int width = 360)
    {
        return addDarkControl(::CreateWindowExW(
            0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, width, 22, parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr));
    }

    HWND addCombo(HWND parent, int id, int x, int y, std::initializer_list<const wchar_t*> items)
    {
        HWND combo = addDarkControl(::CreateWindowExW(
            0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, x, y, 300, 160,
            parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), core_api::get_my_instance(), nullptr));
        for (const wchar_t* item : items)
            ::SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        return combo;
    }

    template <std::size_t N> int addTextFields(HWND page, const TextFieldSetting (&fields)[N], int y)
    {
        for (const auto& field : fields)
        {
            addLabel(page, field.label, 10, y);
            edits_[field.id] = addEdit(page, field.id, 170, y);
            y += 30;
        }
        return y;
    }

    void insertTab(int index, const wchar_t* label)
    {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t*>(label);
        TabCtrl_InsertItem(tabs_, index, &item);
    }

    void createScrobblingPage()
    {
        HWND page = pages_[TabScrobbling];
        addHeader(page, L"Submission Behavior", 10, 10);
        disableNowPlaying_ = addCheckbox(page, IdDisableNowPlaying, L"Disable Now Playing notifications", 170, 40);
        onlyLibrary_ = addCheckbox(page, IdOnlyLibrary, L"Only scrobble from media library", 170, 68);

        addHeader(page, L"Dynamic Sources", 10, 112);
        dynamicLabel_ = addLabel(page, L"Use:", 10, 144);
        dynamicCombo_ = addCombo(page, IdDynamicCombo, 170, 140,
                                 {L"No dynamic sources", L"Only Now Playing", L"Now Playing and scrobbling"});

        addHeader(page, L"Console", 10, 190);
        addLabel(page, L"Log level:", 10, 222);
        consoleCombo_ = addCombo(page, IdConsoleCombo, 170, 218, {L"None", L"Basic", L"Debug"});

        addHeader(page, L"Playback Menu", 10, 264);
        showPlaybackMenu_ = addCheckbox(page, IdShowPlaybackMenu, L"Show Last.fm in the Playback menu", 170, 294);

        addHeader(page, L"Authentication", 10, 338);
        addLabel(page, L"Status:", 10, 370);
        authStatus_ = addDarkControl(::CreateWindowExW(
            0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, 170, 373, 420, 40, page,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IdAuthStatus)), core_api::get_my_instance(), nullptr));
    }

    void createTagsPage()
    {
        HWND page = pages_[TabTags];
        addHeader(page, L"Tag Formatting", 10, 10);
        const int y = addTextFields(page, kTagFields, 40);
        treatVariousArtists_ = addCheckbox(page, IdTreatVariousArtists,
                                           L"Treat \"Various Artists\" as empty for album artist", 170, y + 8, 420);
    }

    void createExclusionsPage()
    {
        HWND page = pages_[TabExclusions];
        addHeader(page, L"Text or Regex", 10, 10);
        int y = addTextFields(page, kExclusionFields, 40);

        addHeader(page, L"TF Templates", 10, y + 8);
        y += 38;
        for (const auto& t : kTemplates)
        {
            templateCheckboxes_[templateIndex(t.checkboxId)] =
                addCheckbox(page, t.checkboxId, t.label, 170, y, kTemplateCheckboxWidth);
            edits_[t.editId] = addEdit(page, t.editId, 170 + kTemplateCheckboxWidth + kTemplateControlGap, y);
            y += 30;
        }
    }

    void layoutPages(int width)
    {
        for (const auto& item : edits_)
            if (item.second)
            {
                RECT rc{};
                ::GetWindowRect(item.second, &rc);
                ::MapWindowPoints(nullptr, ::GetParent(item.second), reinterpret_cast<POINT*>(&rc), 2);

                const int fieldWidth = std::max(1, width - static_cast<int>(rc.left) - kEditRightMargin);
                ::MoveWindow(item.second, rc.left, rc.top, fieldWidth, rc.bottom - rc.top, TRUE);
            }

        if (authStatus_)
            ::MoveWindow(authStatus_, 170, 373, std::max(240, width - 210), 40, TRUE);
    }

    void showSelectedTab()
    {
        const int selected = TabCtrl_GetCurSel(tabs_);
        for (int i = 0; i < TabCount; ++i)
            if (i != selected)
                ::ShowWindow(pages_[i], SW_HIDE);

        if (selected >= 0 && selected < TabCount)
        {
            ::SetWindowPos(pages_[selected], HWND_TOP, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            ::RedrawWindow(pages_[selected], nullptr, nullptr,
                           RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
    }

    void loadSettings()
    {
        loading_ = true;
        setComboSelection(consoleCombo_, lastfm::settings::consoleLevel());
        setChecked(showPlaybackMenu_, lastfm::settings::showPlaybackMenu());
        setChecked(disableNowPlaying_, lastfm::settings::disableNowPlaying());
        setChecked(onlyLibrary_, lastfm::settings::onlyScrobbleFromMediaLibrary());
        setComboSelection(dynamicCombo_, lastfm::settings::configuredDynamicSourcesMode());
        setChecked(treatVariousArtists_, lastfm::settings::treatVariousArtistsAsEmpty());

        forEachTextSetting([this](int id, const auto& setting) { setText(id, setting.getValue()); });

        refreshTemplateValueCache();
        refreshTemplateCheckboxes();
        refreshDynamicEnabledState();
        loading_ = false;
    }

    bool hasChanged() const
    {
        bool changed = comboSelection(consoleCombo_) != lastfm::settings::consoleLevel() ||
                       (lastfmGetAuthState().isAuthenticated &&
                        checked(showPlaybackMenu_) != lastfm::settings::showPlaybackMenu()) ||
                       checked(disableNowPlaying_) != lastfm::settings::disableNowPlaying() ||
                       checked(onlyLibrary_) != lastfm::settings::onlyScrobbleFromMediaLibrary() ||
                       comboSelection(dynamicCombo_) != lastfm::settings::configuredDynamicSourcesMode() ||
                       checked(treatVariousArtists_) != lastfm::settings::treatVariousArtistsAsEmpty();
        forEachTextSetting([this, &changed](int id, const auto& setting)
                           { changed = changed || getText(id) != setting.getValue(); });
        return changed;
    }

    void refreshAuthStatus()
    {
        const LastfmAuthState state = lastfmGetAuthState();
        const std::string text = state.isAuthenticated
                                     ? "Authenticated as " + state.username + "."
                                     : "User not authenticated, please authenticate\nfrom the Playback menu.";
        setWindowUtf8(authStatus_, text);

        ::EnableWindow(showPlaybackMenu_, state.isAuthenticated ? TRUE : FALSE);
        if (state.isAuthenticated != showPlaybackMenuAuthed_)
        {
            showPlaybackMenuAuthed_ = state.isAuthenticated;
            // The menu is always visible while logged out, so show the box checked; restore the
            // stored choice once authenticated again.
            setChecked(showPlaybackMenu_, state.isAuthenticated ? lastfm::settings::showPlaybackMenu() : true);
            notifyChanged();
        }
    }

    void refreshDynamicEnabledState()
    {
        const BOOL enabled = checked(onlyLibrary_) ? FALSE : TRUE;
        ::EnableWindow(dynamicLabel_, enabled);
        ::EnableWindow(dynamicCombo_, enabled);
    }

    void refreshTemplateValueCache()
    {
        for (std::size_t i = 0; i < std::size(kTemplates); ++i)
            templateValues_[i] = getText(kTemplates[i].editId);
    }

    void refreshTemplateCheckboxes()
    {
        const std::string tf = getText(IdExcludeTf);
        for (std::size_t i = 0; i < std::size(kTemplates); ++i)
        {
            const auto& t = kTemplates[i];
            setChecked(templateCheckboxes_[i], hasTemplateExpression(tf, templateExpression(t, getText(t.editId))));
        }
    }

    void updateTemplateExpressionFromControl(int id)
    {
        const int index = templateIndex(id);
        if (index < 0)
            return;

        const auto& t = kTemplates[index];
        const std::string oldExpr = templateExpression(t, templateValues_[index]);
        const std::string newValue = getText(t.editId);
        const std::string newExpr = templateExpression(t, newValue);

        std::string tf = getText(IdExcludeTf);
        tf = removeTemplateExpression(tf, oldExpr);

        bool enabled = checked(templateCheckboxes_[index]);
        if (enabled && !newExpr.empty())
            tf = appendTemplateExpression(tf, newExpr);
        else
            enabled = false;

        templateValues_[index] = newValue;
        setText(IdExcludeTf, tf);
        setChecked(templateCheckboxes_[index], enabled);
    }

    int templateIndex(int id) const
    {
        for (std::size_t i = 0; i < std::size(kTemplates); ++i)
            if (kTemplates[i].checkboxId == id || kTemplates[i].editId == id)
                return static_cast<int>(i);
        return -1;
    }

    std::string getText(int id) const
    {
        auto it = edits_.find(id);
        return it == edits_.end() ? std::string{} : utf8FromWindow(it->second);
    }

    void setText(int id, const std::string& value)
    {
        auto it = edits_.find(id);
        if (it != edits_.end())
            setWindowUtf8(it->second, value);
    }

    void notifyChanged()
    {
        callback_->on_state_changed();
    }

    preferences_page_callback::ptr callback_;
    bool loading_ = false;
    HWND tabs_ = nullptr;
    std::array<HWND, TabCount> pages_{};
    std::map<int, HWND> edits_;
    std::vector<HWND> headers_;
    std::array<HWND, std::size(kTemplates)> templateCheckboxes_{};
    std::array<std::string, std::size(kTemplates)> templateValues_{};
    HWND consoleCombo_ = nullptr;
    HWND authStatus_ = nullptr;
    HWND disableNowPlaying_ = nullptr;
    HWND onlyLibrary_ = nullptr;
    HWND dynamicLabel_ = nullptr;
    HWND dynamicCombo_ = nullptr;
    HWND treatVariousArtists_ = nullptr;
    HWND showPlaybackMenu_ = nullptr;
    bool showPlaybackMenuAuthed_ = true;
    HFONT boldFont_ = nullptr;
    fb2k::CCoreDarkModeHooks dark_;
};

class lastfm_preferences_page : public preferences_page_impl<LastfmPreferencesPage>
{
  public:
    const char* get_name() override
    {
        return "Foo Scrobbler";
    }
    GUID get_guid() override
    {
        return GUID_LASTFM_PREFERENCES_PAGE;
    }
    GUID get_parent_guid() override
    {
        return preferences_page::guid_tools;
    }
};

preferences_page_factory_t<lastfm_preferences_page> g_lastfmPreferencesPageFactory;
} // namespace
