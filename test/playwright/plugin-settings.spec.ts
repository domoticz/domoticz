import { test, expect } from '@playwright/test';

// These tests require a running Domoticz instance on port 8080
// with the TestExtendedSettings plugin installed and trusted networks
// configured for localhost.

const PLUGIN_KEY = 'TestExtSettings';

test.describe('Extended Plugin Settings UI', () => {

	test.beforeEach(async ({ page, context }) => {
		// Disable caching to ensure fresh JS files
		await page.route('**/*.js', (route) => {
			route.continue({ headers: { ...route.request().headers(), 'Cache-Control': 'no-cache' } });
		});
		await page.goto('/', { waitUntil: 'networkidle' });
		await page.waitForTimeout(1000);

		// Complete setup wizard if shown
		const setupForm = page.locator('text=Create your admin account');
		if (await setupForm.isVisible({ timeout: 2000 }).catch(() => false)) {
			await page.fill('input[placeholder="Password"]', 'testpass123');
			await page.fill('input[placeholder="Confirm Password"]', 'testpass123');
			await page.click('button:has-text("Create Account")');
			await page.waitForTimeout(2000);
		}

		// Navigate to Hardware page via Setup menu
		await page.click('text=Setup');
		await page.waitForTimeout(500);
		await page.click('a[href*="Hardware"]');
		await page.waitForSelector('#hardwareparamstable #combotype', { timeout: 10000 });
	});

	test('plugin appears in hardware type dropdown', async ({ page }) => {
		const option = page.locator(`#hardwareparamstable #combotype option[value="${PLUGIN_KEY}"]`).first();
		await expect(option).toBeAttached();
		await expect(option).toContainText('Test Extended Settings Plugin');
	});

	test('selecting plugin shows all parameter fields', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		// Reserved fields: Address, Port
		await expect(pluginDiv.locator('#Address').first()).toBeVisible();
		await expect(pluginDiv.locator('#Port').first()).toBeVisible();

		// Number input: Interval
		const interval = pluginDiv.locator('#Interval').first();
		await expect(interval).toBeVisible();
		await expect(interval).toHaveAttribute('type', 'number');
		await expect(interval).toHaveAttribute('min', '5');
		await expect(interval).toHaveAttribute('max', '3600');
		await expect(interval).toHaveAttribute('step', '5');
		await expect(interval).toHaveValue('30');

		// Boolean input: EnableDebug — checkbox may be tiny, check it's in DOM with correct attributes
		const enableDebug = pluginDiv.locator('#EnableDebug').first();
		await expect(enableDebug).toBeAttached();
		await expect(enableDebug).toHaveAttribute('type', 'checkbox');
		expect(await enableDebug.isChecked()).toBe(false);

		// Slider: Brightness
		const sliderDiv = pluginDiv.locator('#slider_Brightness').first();
		await expect(sliderDiv).toBeVisible();
		const sliderInput = pluginDiv.locator('#Brightness').first();
		await expect(sliderInput).toHaveValue('50');
		const sliderLabel = pluginDiv.locator('#sliderval_Brightness').first();
		await expect(sliderLabel).toHaveText('50');

		// Select: Protocol
		const protocol = pluginDiv.locator('#Protocol').first();
		await expect(protocol).toBeVisible();
		await expect(protocol).toHaveValue('http');

		// Password field: ApiKey
		const apiKey = pluginDiv.locator('#ApiKey').first();
		await expect(apiKey).toBeVisible();
		await expect(apiKey).toHaveAttribute('type', 'password');

		// Legacy Mode6 select
		const mode6 = pluginDiv.locator('#Mode6').first();
		await expect(mode6).toBeVisible();

		await page.screenshot({ path: '/tmp/pw-plugin-fields.png' });
	});

	test('conditional visibility hides Certificate when HTTP selected', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		// Test conditional visibility using page.evaluate to find the table
		// with active change handlers (Angular.js may create duplicate tables)
		const result = await page.evaluate((key) => {
			const $ = (window as any).$;
			const tables = document.querySelectorAll(`#divpythonplugin #${key}`);
			let $table: any = null;
			for (let i = 0; i < tables.length; i++) {
				const p = tables[i].querySelector('#Protocol');
				if (p && $._data(p, 'events')?.change?.length > 0) {
					$table = $(tables[i]);
					break;
				}
			}
			if (!$table) return { error: 'No table with change handler found' };

			const $protocol = $table.find('#Protocol');
			const $certRow = $table.find('tr[data-visible-when="Protocol=https"]');

			const initialDisplay = $certRow.css('display');
			$protocol.val('https').trigger('change');
			const afterHttps = $certRow.css('display');
			$protocol.val('http').trigger('change');
			const afterHttp = $certRow.css('display');

			return { initialDisplay, afterHttps, afterHttp };
		}, PLUGIN_KEY);

		expect(result).not.toHaveProperty('error');
		expect(result.initialDisplay).toBe('none');
		expect(result.afterHttps).not.toBe('none');
		expect(result.afterHttp).toBe('none');
	});

	test('collapsible group section works', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		// Group header should be visible
		const groupHeader = pluginDiv.locator('.plugin-group:has-text("Advanced Settings")').first();
		await expect(groupHeader).toBeVisible();

		// Group content should be hidden initially
		const groupContent = groupHeader.locator('+ div');
		await expect(groupContent).toBeHidden();

		// Click to expand
		await groupHeader.click();
		await expect(groupContent).toBeVisible();

		// Group fields should now be in the DOM with correct defaults
		const retryCount = pluginDiv.locator('#RetryCount').first();
		await expect(retryCount).toBeAttached();
		await expect(retryCount).toHaveValue('3');

		const timeout = pluginDiv.locator('#Timeout').first();
		await expect(timeout).toBeAttached();
		await expect(timeout).toHaveValue('10');

		const enableNotif = pluginDiv.locator('#EnableNotifications').first();
		await expect(enableNotif).toBeAttached();
		expect(await enableNotif.isChecked()).toBe(true);

		// Click to collapse
		await groupHeader.click();
		await expect(groupContent).toBeHidden();

		await page.screenshot({ path: '/tmp/pw-group-collapsed.png' });
	});

	test('number input respects min/max constraints', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const interval = page.locator(`#divpythonplugin #${PLUGIN_KEY} #Interval`).first();

		await interval.fill('60');
		await expect(interval).toHaveValue('60');

		await expect(interval).toHaveAttribute('min', '5');
		await expect(interval).toHaveAttribute('max', '3600');
		await expect(interval).toHaveAttribute('step', '5');
	});

	test('save and restore plugin settings round-trip', async ({ page, request }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		// Fill in a hardware name
		await page.fill('#hardwareparamstable #hardwarename', 'Playwright Test Plugin');

		// Set some custom values
		await pluginDiv.locator('#Address').first().fill('192.168.1.100');
		await pluginDiv.locator('#Port').first().fill('9090');
		await pluginDiv.locator('#Interval').first().fill('120');
		// Use evaluate to check the checkbox since it may not be visible to Playwright
		await pluginDiv.locator('#EnableDebug').first().evaluate((el: HTMLInputElement) => {
			el.checked = true;
			el.dispatchEvent(new Event('change'));
		});

		// Save the hardware (click Add button)
		await page.click('a[onclick="AddHardware();"]');
		await page.waitForTimeout(2000);

		// Verify via API that settings were saved (use page.evaluate to share session)
		const data = await page.evaluate(async () => {
			const resp = await fetch('/json.htm?type=command&param=gethardware');
			return resp.json();
		});
		const hw = data.result?.find((h: any) => h.Name === 'Playwright Test Plugin');

		expect(hw).toBeDefined();
		expect(hw.Address).toBe('192.168.1.100');
		expect(hw.Settings).toBeDefined();
		expect(hw.Settings.Interval).toBe('120');
		expect(hw.Settings.EnableDebug).toBe('true');

		// Cleanup: remove the test hardware
		if (hw.idx) {
			await page.evaluate(async (idx: string) => {
				await fetch(`/json.htm?type=command&param=deletehardware&idx=${idx}`);
			}, hw.idx);
		}

		await page.screenshot({ path: '/tmp/pw-save-roundtrip.png' });
	});
});
