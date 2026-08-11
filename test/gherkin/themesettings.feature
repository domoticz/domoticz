Feature: Per-user theme settings
    The themesettings commands store instance-wide defaults (admin only) and
    per-user overlays (any authenticated user). getsettings returns the merged
    view for the calling user, user rows winning per theme.

    Background:
        Given Domoticz is running with themesettings support
        And an authenticated admin session
        And a viewer user "bddviewer" with an authenticated session

    Scenario: Admin stores an instance-wide default
        When the admin stores the instance default '{"style": "dark", "size": 1}' for theme "bddtheme"
        Then the write is accepted
        And themesettings_get for "bddtheme" as viewer shows the instance layer present
        And themesettings_get for "bddtheme" as viewer shows the user layer absent

    Scenario: Viewer overlay wins in the merged getsettings view
        Given the admin stored the instance default '{"style": "dark", "size": 1}' for theme "bddtheme"
        When the viewer stores the overlay '{"style": "light"}' for theme "bddtheme"
        Then getsettings as viewer returns ThemeSettings "bddtheme" equal to '{"style": "light"}'
        And getsettings as admin returns ThemeSettings "bddtheme" equal to '{"size": 1, "style": "dark"}'

    Scenario: Viewer cannot write the instance scope
        When the viewer tries to store an instance default for theme "bddtheme"
        Then the request is rejected with HTTP status 403

    Scenario: A stale concurrency token is rejected
        Given the viewer stored the overlay '{"style": "light"}' for theme "bddtheme"
        When the viewer stores the overlay '{"style": "blue"}' for theme "bddtheme" with token "1999-01-01 00:00:00.000"
        Then the write is rejected with error "conflict"

    Scenario: Reset removes the overlay and restores the default
        Given the admin stored the instance default '{"style": "dark", "size": 1}' for theme "bddtheme"
        And the viewer stored the overlay '{"style": "light"}' for theme "bddtheme"
        When the viewer resets theme "bddtheme"
        Then getsettings as viewer returns ThemeSettings "bddtheme" equal to '{"size": 1, "style": "dark"}'

    Scenario: reset=all clears every overlay the calling user holds
        Given the admin stored the instance default '{"style": "dark", "size": 1}' for theme "bddtheme"
        And the viewer stored the overlay '{"style": "light"}' for theme "bddtheme"
        And the viewer stored the overlay '{"style": "blue"}' for theme "bddtheme2"
        And the admin stored the overlay '{"style": "admins own"}' for theme "bddtheme"
        When the viewer clears all their theme settings
        Then themesettings_get for "bddtheme" as viewer shows the user layer absent
        And themesettings_get for "bddtheme2" as viewer shows the user layer absent
        And getsettings as viewer returns ThemeSettings "bddtheme" equal to '{"size": 1, "style": "dark"}'
        And getsettings as admin returns ThemeSettings "bddtheme" equal to '{"style": "admins own"}'

    Scenario: A non-object value is rejected
        When the viewer stores the overlay '[1, 2, 3]' for theme "bddtheme"
        Then the write is rejected with error "invalid_json"

    Scenario: Writes require POST
        When the viewer sends the overlay for theme "bddtheme" as a GET request
        Then the write is rejected with error "post_required"

    Scenario: getversion advertises the capability
        Then getversion as viewer contains ThemeSettingsAPI 1

    Scenario: Deleting a user deletes their overlay rows
        Given a temporary user "bddtemp" stored the overlay '{"a": 1}' for theme "bddtheme"
        When the admin deletes user "bddtemp" and recreates it
        Then themesettings_get for "bddtheme" as the recreated user shows the user layer absent

    Scenario: The per-scope theme cap is enforced
        When the viewer stores 33 distinct overlay themes
        Then the 33rd write is rejected with error "too_many_themes"
