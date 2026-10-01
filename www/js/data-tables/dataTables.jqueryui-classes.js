/*
 * jQuery UI class map for DataTables.
 *
 * DataTables removed the bJQueryUI option and its built-in ext.oJUIClasses map after 1.10, but
 * 1.13 still resolves the "H"/"F" dom shorthand through sJUIHeader/sJUIFooter, still computes
 * sSortingClassJUI from the sSortJUI* slots, and still ships the jqueryui header renderer. Those
 * class slots simply default to empty strings now.
 *
 * www/css/demo_table_jui.css and the ui-darkness jQuery UI theme style every list table through
 * exactly those classes, so this file fills the slots with the values DataTables 1.10.15 used and
 * selects the jqueryui renderer. The rendered markup stays identical, and no table's own options
 * have to change.
 *
 * Load after jquery.dataTables.min.js. Drop this file once the tables are styled without
 * jQuery UI.
 */
(function ($) {
	var DataTable = $.fn.dataTable;

	var state = 'ui-state-default';
	var icon = 'css_right ui-icon ui-icon-';
	var toolbar = 'fg-toolbar ui-toolbar ui-widget-header ui-helper-clearfix';

	$.extend(DataTable.ext.classes, {
		sPageButton: 'fg-button ui-button ' + state,
		sPageButtonActive: 'ui-state-disabled',
		sPageButtonDisabled: 'ui-state-disabled',
		sPaging: 'dataTables_paginate fg-buttonset ui-buttonset fg-buttonset-multi ui-buttonset-multi paging_',
		sSortAsc: state + ' sorting_asc',
		sSortDesc: state + ' sorting_desc',
		sSortable: state + ' sorting',
		sSortableAsc: state + ' sorting_asc_disabled',
		sSortableDesc: state + ' sorting_desc_disabled',
		sSortableNone: state + ' sorting_disabled',
		sSortJUIAsc: icon + 'triangle-1-n',
		sSortJUIDesc: icon + 'triangle-1-s',
		sSortJUI: icon + 'carat-2-n-s',
		sSortJUIAscAllowed: icon + 'carat-1-n',
		sSortJUIDescAllowed: icon + 'carat-1-s',
		sSortJUIWrapper: 'DataTables_sort_wrapper',
		sSortIcon: 'DataTables_sort_icon',
		sScrollHead: 'dataTables_scrollHead ' + state,
		sScrollFoot: 'dataTables_scrollFoot ' + state,
		sHeaderTH: state,
		sFooterTH: state,
		sJUIHeader: toolbar + ' ui-corner-tl ui-corner-tr',
		sJUIFooter: toolbar + ' ui-corner-bl ui-corner-br'
	});

	// Only the header renderer has a jqueryui variant; pagination falls back to the default one,
	// which is what 1.10 did as well.
	DataTable.defaults.renderer = 'jqueryui';
})(jQuery);
