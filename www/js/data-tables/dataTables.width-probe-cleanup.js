/*
 * Remove the column-width probes DataTables 1.13 leaves behind.
 *
 * To size columns, DataTables 1.13 appends one temporary <div> per column with a declared width
 * to the table container, reads its offsetWidth, and then calls $(probes).remove() on an ARRAY OF
 * JQUERY OBJECTS. jQuery cannot resolve those to elements, so nothing is removed and every sizing
 * pass leaves the probes in the wrapper. They are empty and unstyled apart from a width, so they
 * are invisible, but a window resize repeats the pass and the wrapper keeps growing.
 *
 * DataTables 2 rewrote the column sizing code and does not have the problem; 1.13.11 is the last
 * release on the 1.x line, so there is no version to upgrade to. Delete this file with the move
 * to DataTables 2.
 *
 * Load after jquery.dataTables.min.js.
 */
(function ($) {
	function removeWidthProbes(settings) {
		// A probe carries an inline width and nothing else; every element DataTables means to keep
		// in the wrapper is classed (the toolbars, the table, the processing indicator).
		$(settings.nTableWrapper).children('div[style]:empty:not([class])').remove();
	}

	$(document).on('init.dt column-sizing.dt', function (event, settings) {
		if (event.namespace === 'dt') {
			removeWidthProbes(settings);
		}
	});
})(jQuery);
