/**
 * Device Detection Service
 *
 * Provides comprehensive device type detection and dashboard view selection.
 * Detects mobile, tablet, and desktop devices based on user-agent and supports
 * manual override via localStorage.
 *
 * Features:
 * - User-agent based device detection (mobile, tablet, desktop)
 * - Manual override support via localStorage
 * - Server configuration (DashboardType) integration
 * - Backward compatibility with $.myglobals.ismobile
 *
 * Priority order for dashboard type selection:
 * 1. Manual override (localStorage.dashboardViewOverride)
 * 2. Server configuration (window.myglobals.DashboardType)
 * 3. User-agent detection (fallback)
 *
 * @module deviceDetection
 */
define(['app'], function(app) {
    app.factory('deviceDetection', function() {
        // Comprehensive mobile device regex pattern
        var mobileRegex = /(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|Android|Silk|lge |maemo|midp|mmp|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino/i;

        // Tablet detection pattern (includes iPad, Android tablets)
        var tabletRegex = /ipad|android(?!.*mobile)|tablet|playbook|silk/i;

        return {
            /**
             * Detect if current device is mobile based on user-agent
             * @returns {boolean} True if mobile device detected
             */
            isMobile: function() {
                return mobileRegex.test(navigator.userAgent || navigator.vendor || window.opera);
            },

            /**
             * Detect if current device is a tablet
             * @returns {boolean} True if tablet device detected
             */
            isTablet: function() {
                return tabletRegex.test(navigator.userAgent || navigator.vendor || window.opera);
            },

            /**
             * Get device type based on user-agent detection only
             * @returns {string} 'mobile', 'tablet', or 'desktop'
             */
            getDeviceType: function() {
                if (this.isMobile()) {
                    return 'mobile';
                } else if (this.isTablet()) {
                    return 'tablet';
                }
                return 'desktop';
            },

            /**
             * Get effective dashboard type considering all factors:
             * 1. Manual override from localStorage
             * 2. Server configuration (DashboardType)
             * 3. User-agent detection
             *
             * @returns {string} 'mobile' or 'desktop'
             */
            getEffectiveType: function() {
                // Check for manual override first
                var override = localStorage.getItem('dashboardViewOverride');
                if (override === 'mobile' || override === 'desktop') {
                    return override;
                }

                // Check server configuration (DashboardType)
                // DashboardType: 0=3col desktop, 1=4col desktop, 2=mobile, 3=floorplan
                if (typeof window.myglobals !== 'undefined' && typeof window.myglobals.DashboardType !== 'undefined') {
                    if (window.myglobals.DashboardType === 2) {
                        return 'mobile';
                    }
                    // Types 0, 1, 3 are desktop modes
                    if (window.myglobals.DashboardType === 0 || window.myglobals.DashboardType === 1 || window.myglobals.DashboardType === 3) {
                        return 'desktop';
                    }
                }

                // Check legacy $.myglobals.ismobile flag
                if (typeof window.myglobals !== 'undefined' && window.myglobals.ismobile === true) {
                    return 'mobile';
                }

                // Fall back to user-agent detection
                var deviceType = this.getDeviceType();
                return (deviceType === 'mobile') ? 'mobile' : 'desktop';
            },

            /**
             * Set manual override for dashboard view
             * @param {string} type - 'mobile' or 'desktop'
             */
            setOverride: function(type) {
                if (type === 'mobile' || type === 'desktop') {
                    localStorage.setItem('dashboardViewOverride', type);
                }
            },

            /**
             * Clear manual override, revert to automatic detection
             */
            clearOverride: function() {
                localStorage.removeItem('dashboardViewOverride');
            },

            /**
             * Get current override value if set
             * @returns {string|null} Current override or null
             */
            getOverride: function() {
                return localStorage.getItem('dashboardViewOverride');
            }
        };
    });

    return app;
});
