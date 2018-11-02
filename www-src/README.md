# Domoticz Web UI

The Domoticz Web UI is made with Angular framework.

You will find below some useful tips to get you started as a developer.

## How to develop locally?

### Install NPM and Angular CLI

Install NPM following official installation guidelines.

Then install Angular CLI with the following command:
```
npm install -g @angular/cli
```
You should now have a command called `ng` available.

### Run the Angular dev server

Start a dev server by running:
```
ng serve
```

Navigate to `http://localhost:4200/`.

The app will automatically reload if you change any of the source files.

### Targeting a running API

Assuming you have a Domoticz server running on your local network, you can
target it for API calls that the UI will make.

This is handled by the `proxy.conf.json` file with defines which requests the
Angular dev server should route to somewhere else.

Replace all occurences of `http://192.168.1.19:8080` by the URL of your Domoticz
instance and start the Angular dev server.

Note #1: any change to this file needs a restart of Angular dev server.

Note #2: this file is only used when running Angular dev server. It is not used with the
compiled production version of the app.

## Adding a new component or service

For instance you can run following command to create a new component in the admin directory:
```
ng generate component admin/something --module=app
```

You can also use `ng generate directive|pipe|service|class|guard|interface|enum|module`.

## Running unit tests

Run `ng test` to execute the unit tests via [Karma](https://karma-runner.github.io).

### Running a single test

You can prefix a test with the letter `f`, like `fdescribe` or `fit` to execute only one test or one file.

## Tips

### How to make links in Datatables

The Datatable library is a plain old Javascript library not integrated to Angular. This means that you
cannot use Angular directives inside Datatable generated HTML.

A workaround is to create a classic HTML link like the following in the Datatable generated HTML:
```typescript
const link = '<a class="ng-link" href="/Devices/' + idx + '">Link</a>'
```

Then, add the following code after the datatable drawing:
```typescript
DatatableHelper.fixAngularLinks('.ng-link', this.router);
```

You can find examples in the project by searching for the method `fixAngularLinks`.
