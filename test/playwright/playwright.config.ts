import { defineConfig } from '@playwright/test';

export default defineConfig({
	testDir: '.',
	timeout: 30000,
	use: {
		baseURL: 'http://localhost:8080',
		headless: true,
		bypassCSP: true,
		// Disable browser cache to pick up JS changes
		launchOptions: {
			args: ['--disable-web-security', '--disable-features=IsolateOrigins', '--disable-site-isolation-trials'],
		},
	},
});
