import { test, expect } from '@playwright/test';

const PLUGIN_KEY = 'TestExtSettings';

test.describe('Extended Plugin Settings — Edge Cases', () => {

	test.beforeEach(async ({ page, context }) => {
		await page.route('**/*.js', (route) => {
			route.continue({ headers: { ...route.request().headers(), 'Cache-Control': 'no-cache' } });
		});
		await page.goto('/', { waitUntil: 'networkidle' });
		await page.waitForTimeout(1000);
		const setupForm = page.locator('text=Create your admin account');
		if (await setupForm.isVisible({ timeout: 2000 }).catch(() => false)) {
			await page.fill('input[placeholder="Password"]', 'testpass123');
			await page.fill('input[placeholder="Confirm Password"]', 'testpass123');
			await page.click('button:has-text("Create Account")');
			await page.waitForTimeout(2000);
		}
		await page.click('text=Setup');
		await page.waitForTimeout(500);
		await page.click('a[href*="Hardware"]');
		await page.waitForSelector('#hardwareparamstable #combotype', { timeout: 10000 });
	});

	test('collapsed group fields are included in saved settings', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		// Fill name
		await page.fill('#hardwareparamstable #hardwarename', 'Edge_CollapsedGroup');

		// Expand the group, fill a value, then collapse it
		const groupHeader = pluginDiv.locator('.plugin-group:has-text("Advanced Settings")').first();
		await groupHeader.click();
		await page.waitForTimeout(300);

		// The group fields should be visible now — fill RetryCount
		await pluginDiv.locator('#RetryCount').first().fill('7');

		// Collapse the group
		await groupHeader.click();
		await page.waitForTimeout(300);

		// Save
		await page.click('a[onclick="AddHardware();"]');
		await page.waitForTimeout(2000);

		// Verify the collapsed group field was saved
		const data = await page.evaluate(async () => {
			const resp = await fetch('/json.htm?type=command&param=gethardware');
			return resp.json();
		});
		const hw = data.result?.find((h: any) => h.Name === 'Edge_CollapsedGroup');
		expect(hw).toBeDefined();
		// Collapsed group fields should still be saved (they're not hidden by visibility, just collapsed)
		expect(hw.Settings.RetryCount).toBe('7');

		// Cleanup
		if (hw.idx) {
			await page.evaluate(async (idx: string) => {
				await fetch(`/json.htm?type=command&param=deletehardware&idx=${idx}`);
			}, hw.idx);
		}
	});

	test('conditional visibility field is excluded when hidden', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		await page.fill('#hardwareparamstable #hardwarename', 'Edge_HiddenField');

		// Protocol is HTTP (default) — Certificate field should be hidden and excluded
		await page.click('a[onclick="AddHardware();"]');
		await page.waitForTimeout(2000);

		const data = await page.evaluate(async () => {
			const resp = await fetch('/json.htm?type=command&param=gethardware');
			return resp.json();
		});
		const hw = data.result?.find((h: any) => h.Name === 'Edge_HiddenField');
		expect(hw).toBeDefined();
		// Certificate should NOT be in Settings since it was hidden
		expect(hw.Settings).not.toHaveProperty('Certificate');

		if (hw.idx) {
			await page.evaluate(async (idx: string) => {
				await fetch(`/json.htm?type=command&param=deletehardware&idx=${idx}`);
			}, hw.idx);
		}
	});

	test('edit restores slider value and updates visual position', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		await page.fill('#hardwareparamstable #hardwarename', 'Edge_SliderRestore');

		// Change slider to 75 via the hidden input
		await pluginDiv.locator('#Brightness').first().evaluate((el: HTMLInputElement) => {
			el.value = '75';
		});

		await page.click('a[onclick="AddHardware();"]');
		await page.waitForTimeout(2000);

		// Now click on the hardware entry in the table to edit it
		const row = page.locator('#hardwaretable td:has-text("Edge_SliderRestore")').first();
		await row.click();
		await page.waitForTimeout(1500);

		// Check slider value was restored
		const sliderVal = await pluginDiv.locator('#Brightness').first().inputValue();
		expect(sliderVal).toBe('75');

		// Check label shows 75
		const labelText = await pluginDiv.locator('#sliderval_Brightness').first().textContent();
		expect(labelText).toBe('75');

		// Cleanup
		const data = await page.evaluate(async () => {
			const resp = await fetch('/json.htm?type=command&param=gethardware');
			return resp.json();
		});
		const hw = data.result?.find((h: any) => h.Name === 'Edge_SliderRestore');
		if (hw?.idx) {
			await page.evaluate(async (idx: string) => {
				await fetch(`/json.htm?type=command&param=deletehardware&idx=${idx}`);
			}, hw.idx);
		}
	});

	test('edit restores conditional visibility state', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		await page.fill('#hardwareparamstable #hardwarename', 'Edge_VisRestore');

		// First add with Protocol=https and a Certificate value
		// Need to find the table with handlers for visibility
		await page.evaluate((key) => {
			const $ = (window as any).$;
			const tables = document.querySelectorAll(`#divpythonplugin #${key}`);
			for (let i = 0; i < tables.length; i++) {
				const p = tables[i].querySelector('#Protocol') as HTMLSelectElement;
				if (p && $._data(p, 'events')?.change?.length > 0) {
					$(p).val('https').trigger('change');
					const cert = tables[i].querySelector('#Certificate') as HTMLInputElement;
					if (cert) cert.value = '/path/to/cert.pem';
					break;
				}
			}
		}, PLUGIN_KEY);
		await page.waitForTimeout(500);

		await page.click('a[onclick="AddHardware();"]');
		await page.waitForTimeout(2000);

		// Click on the saved entry to edit
		const row = page.locator('#hardwaretable td:has-text("Edge_VisRestore")').first();
		await row.click();
		await page.waitForTimeout(1500);

		// Protocol should be restored to HTTPS
		const data = await page.evaluate(async () => {
			const resp = await fetch('/json.htm?type=command&param=gethardware');
			return resp.json();
		});
		const hw = data.result?.find((h: any) => h.Name === 'Edge_VisRestore');
		expect(hw).toBeDefined();
		expect(hw.Settings.Protocol).toBe('https');
		expect(hw.Settings.Certificate).toBe('/path/to/cert.pem');

		// Cleanup
		if (hw?.idx) {
			await page.evaluate(async (idx: string) => {
				await fetch(`/json.htm?type=command&param=deletehardware&idx=${idx}`);
			}, hw.idx);
		}
	});

	test('autocomplete=off is set on text and number inputs', async ({ page }) => {
		await page.selectOption('#hardwareparamstable #combotype', PLUGIN_KEY);
		await page.waitForSelector(`#divpythonplugin #${PLUGIN_KEY}`, { timeout: 5000 });

		const pluginDiv = page.locator(`#divpythonplugin #${PLUGIN_KEY}`).first();

		// Text input
		await expect(pluginDiv.locator('#Address').first()).toHaveAttribute('autocomplete', 'off');
		// Number input
		await expect(pluginDiv.locator('#Interval').first()).toHaveAttribute('autocomplete', 'off');
		// Password input
		await expect(pluginDiv.locator('#ApiKey').first()).toHaveAttribute('autocomplete', 'off');
	});
});
