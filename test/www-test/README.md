# WWW Unit tests
This folder contains the unit tests for components in www.

_Be warned! This is a work in progres._

# Tooling
## mocha
A tool called 'mocha' is used to drive the tests. The tests are written
specifically to be used with mocha. See: https://mochajs.org/

It is not written in stone that mocha be used for unit testing. A different
framework can be used for different kind of tests.

# Setup
## Installation
As the www-components are written in JavaScript, we need a JavaScript 
engine to run on. For example, Node.js. See https://nodejs.org/en/. Make
sure you have Node.js installed. Node.js comes with npm, which is the 
package manager that allows you to install different kind of packages
to use on your Node.js engine. For example, the mocha package.

In short, follow the installation instructions on the mocha website. But
what it says basically is:

`npm install --global mocha`

# Usage
## Structure
main.js - contains the requirejs module configuration. It should register
all the modules that the components under test use.  
_path/component.js_ - the files containing the tests should be in the
same folder structure as the components under test are,
relative to the 'www' folder.

## Running the test
Run the tests in a specific file with:

`mocha _path_/test.js`

For example, there is one test.js file in app/log. Run this with:

`mocha app/log/test.js`

It should display something like this:
    
    
      RefreshingDayChart
        constructor
          √ should be instantiable
    
    
      1 passing (5ms)
    
    device -> idx:1234, type:Type, subtype:SubType
    installed:handler for event type time_update
    
    

If the test runs fine, only the test output is displayed.
If an error occurs, mocha will show the error and complete stacktrace.
 