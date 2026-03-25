// #BuildHash //forces a cache refresh on every new build

importScripts('js/workbox/workbox-sw.js');

workbox.setConfig({ modulePathPrefix: 'js/workbox/' });

// ─── Precache ───
// Offline fallback page (self-contained HTML)
workbox.precaching.precacheAndRoute([
  { url: 'views/offline-standalone.html', revision: null },
  { url: 'views/offline.html', revision: null },
  { url: 'app/OfflineController.js', revision: null },
  { url: 'images/logo/57.png', revision: null },
  { url: 'css/images/img01.jpg', revision: null },
]);
// revision: null is fine because #BuildHash changes the SW on every build,
// which triggers a full precache update

// ─── API: Always network, never cache ───
workbox.routing.registerRoute(
  ({ url }) => url.pathname === '/json.htm',
  new workbox.strategies.NetworkOnly()
);

// ─── WebSocket/live endpoints: Network only ───
workbox.routing.registerRoute(
  ({ url }) => url.pathname === '/ozwcp/cp.html',
  new workbox.strategies.NetworkOnly()
);

// ─── HTML pages (index.html, SPA navigation): Network first ───
// Falls back to cache if offline, ultimate fallback to offline.html
workbox.routing.registerRoute(
  ({ request }) => request.mode === 'navigate',
  new workbox.strategies.NetworkFirst({
    cacheName: 'pages',
    plugins: [
      new workbox.expiration.ExpirationPlugin({ maxEntries: 50 }),
    ],
  })
);

// ─── Static JS & CSS: StaleWhileRevalidate ───
// Serve from cache instantly, update in background.
// Safe because #BuildHash forces SW update on new builds,
// and these files have stable URLs with no content hashing.
workbox.routing.registerRoute(
  ({ request }) => request.destination === 'script' || request.destination === 'style',
  new workbox.strategies.StaleWhileRevalidate({
    cacheName: 'static-assets',
    plugins: [
      new workbox.cacheableResponse.CacheableResponsePlugin({ statuses: [200] }),
      new workbox.expiration.ExpirationPlugin({ maxEntries: 200 }),
    ],
  })
);

// ─── Images: StaleWhileRevalidate ───
// Serve cached image instantly, refresh in background.
// Users can upload custom device icons that replace existing URLs,
// so CacheFirst would serve stale images indefinitely.
workbox.routing.registerRoute(
  ({ request }) => request.destination === 'image',
  new workbox.strategies.StaleWhileRevalidate({
    cacheName: 'images',
    plugins: [
      new workbox.cacheableResponse.CacheableResponsePlugin({ statuses: [200] }),
      new workbox.expiration.ExpirationPlugin({
        maxEntries: 500,
        maxAgeSeconds: 30 * 24 * 60 * 60, // 30 days
      }),
    ],
  })
);

// ─── Fonts: Cache first with long TTL ───
workbox.routing.registerRoute(
  ({ request }) => request.destination === 'font',
  new workbox.strategies.CacheFirst({
    cacheName: 'fonts',
    plugins: [
      new workbox.cacheableResponse.CacheableResponsePlugin({ statuses: [200] }),
      new workbox.expiration.ExpirationPlugin({
        maxEntries: 30,
        maxAgeSeconds: 365 * 24 * 60 * 60, // 1 year
      }),
    ],
  })
);

// ─── i18n locale files: StaleWhileRevalidate ───
workbox.routing.registerRoute(
  ({ url }) => url.pathname.startsWith('/i18n/'),
  new workbox.strategies.StaleWhileRevalidate({
    cacheName: 'i18n',
    plugins: [
      new workbox.expiration.ExpirationPlugin({ maxEntries: 40 }),
    ],
  })
);

// ─── Offline navigation fallback ───
workbox.routing.setCatchHandler(async ({ event }) => {
  if (event.request.mode === 'navigate') {
    return caches.match(workbox.precaching.getCacheKeyForURL('views/offline-standalone.html'));
  }
  return Response.error();
});
