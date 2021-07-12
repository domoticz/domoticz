# Domoticz Security

This document describes the security options within Domoticz

By default, any Domoticz installation is open to the netwerk it is placed in. Usually that would be someone's private network at home, work, school, barn, car, etc. And for many, if not most, use-cases that is sufficient and what we want. The goal is to control things within the local environment.

So it is a __bad__ idea to connect any installation directly to the internet. Because that would mean anyone anywhere can access Domoticz and control connected devices. Could be _fun_ in some edge-cases, but most likely not what anyone would want.

The same applies for _larger_ networks, for example a school or an office, where you do not want just anyone that has access to the network, also get access to Domoticz. Schoolkids would probably love it, but teacher and staff probably not :)


There are a number of 'components'/'functions' involved when it comes to Domoticz security:
* Users
* Trusted networks
* _Clients_
* _Domoticz_
    * The Domoticz __UI__ (should be regarded as a _client_)
    * The Domoticz __resource server__ (core server)
    * The Domoticz __'IAM server'__ (Identity & Access Management)    
        _(actually part of the core server at the moment)_

### Isn't Domoticz just 1 Application?
Yes and No :)

From a User perspective, often it is consired as a single application. Until the point where users start using alternative _'tools'_ to access/control their Domoticz setup.

For example when using an alternative- and/or additional Dashboard. An additional _'application'_ gets installed and is connected to the existing Domoticz installation.

Same goes for example when a user starts to use a mobile native app. The Users installs an additional app on the mobile device and connects it to the Domoticz installation.

From a development architectural perspective, different parts of the Domoticz installation are used in different ways and situation.

The core part, the _resource server_ is always used. It is the heart of the system that holds and controls all devices and data.

Depending on the User(s) environment 1 of more different _User Interfaces (UI's)_ are used to interact with the _resource server_. Each of those different _UI's_ is called a _client_. And a standard Domoticz installation comes with a default _client_, the web _UI_.

When a Domoticz setup becomes either more complex, gets more users, needs connection to the Internet or other less- or insecure networks, it becomes neccessary to add ___SECURITY___. This is where the functions and features of the _IAM server_ comes into play.

### IAM server

Domoticz has a build-in _IAM service_ (part of the core server at the moment) that can be used to handle _Identity_ (Authentication) and _Access_ (Authorization) Management. Here we can define Users and their (access)rights for a (single) Domoticz installation.

But often in larger/complex environments, there usually already is a more centralized _IAM service_, for example a Windows AD (Active Directory) Server.

Also in modern (Hybrid) Cloud environments, a more centralized _IAM service_ is often in place.

Using a centralized _IAM service_, instead of the internal _IAM server_ of each Domoticz installation, makes it much easier the do the on-and offboarding of Users, granting and revoking rights, etc. Also _Single Sign-On_ (SSO) to different applications including one- or more Domoticz installations becomes possible.

And thanks to modern (web)standards like OAuth2, OpenID Connect, etc. different _IAM Services and servers_ become interchangable and interopperable, making the life of sysadmins a little easier.

Due to the now build in support of those standards, Domoticz can become integrated in such environments. 

## Add user-management

A first step is to activate 'User-Management'. User-Management add's a first layer of security by requiring possible users to _identify_ themselves before they get access.

Also by default, when no User-Management is active (meaning no Users are configured), the (anonymous) user automatically get __admin__ privileges.

## Add trusted-networks

Users coming from a _'trusted network'_ do not need to _identify_ themselves.

But when coming from 'outside' a trusted network, Domoticz will require the user to authenticate.

## Add _clients_ 
_(Applications that want to communicate with the Domoticz Server)_

Next to _normal_ User Management, it is also possible to register _clients_. Clients are 'applications' that would like to interact with the core Domoticz server to retrieve information and/or perform actions. These applications should/could do so on behalf of registered Users.

Examples of _clients_ are web applications like Dashticz, Reacticz, mobile apps like Domoticz mobile (Xamarin or Android), Domotix, Pilot Home Control or SaaS services as myDomoticz.

_Why do we need clients?_
Basically by having Users with a username/password, we created a way to secure the use of Domoticz _resources_. A given user can be restricted to do only a given number of actions. But the User uses a _client_ (an application) to perform these actions on his/her behalf. The standard Domoticz UI being one of them.

When the Users wants to use other _clients_ as well, the _clients_ also need the Users username/password to perform actions on Domoticz resources (through interaction with the Domoticz resource server.)

The __problem__ is 'how to trust all these _clients_?'. Putting your Domoticz User credentials into a nice app you just installed on your mobile or found as a SaaS service or installed yourself, etc. makes these credentials open to (un)intentional leakage. Once these credentials are leaked, they could be used by any random _client_.

Therefor it is better to tell the 'Domoticz resource server' which _clients_ are allowed to ask for access on behalf of a User when providing User credentials. 

# Connecting Domoticz to the Internet/Cloud

More and more, Users are looking into ways to control their _Automated Home_ from a distance. So preferably _through the Cloud_. This means that a Domoticz setup somehow needs to be _accesible_ outside the safe environment of the Home.

This means there is a need to have some kind of _application_ (a _client_) that can access the _resource server_ to perform actions or exchange data.

The _resource server_ always has to be _in the Home_ (local) as here it can safely be connected to the devices it controls. Putting the _resource server_ in the Cloud would mean that all communication between Domoticz and the devices would have to go through the Cloud. That is not a wanted scenario as it poses numerous security risks that are almost impossible to mitigate. And probably not even possible with some/most hardware devices.

That leaves on the _client_ as a candidate to make accessible/run in the Cloud.

And to make sure that the _client_ can safely interact with the (local) _resource server_, this has to be secured. Using Identity and Access Management (IAM) is a good way to secure the _resource server_.

# Supporting web standards

Domoticz tries to leverage best practices and (RFC) standards where possible. Sometimes not all details of each standard are fully implemented or honoured, but we should be as close as possible and fitting with Domoticz as possible. Any diversion, etc. should be well documented (and explained probably).

## OAuth2 support (RFC6749)

At the moment a few flows/grant_types are supported:
* __authorization_code__
* __authorization_code with PKCE__
* __password__

The _implicit_ flow is not as it is not secure. But using _authorization_code flow with the PKCE extension_ is also possible now. PKCE stands for _Proof Key for Code Exchange_.

We do not support _client_credentials_ flow as we do not want 'anonymous' access by a client/application.

### Tokens

There are 3 types of _tokens_
* _authorization code_
* _access_token_
* _refresh_token_


### authorization_code flow

This is the flow that should be used in almost all cases.

This flow returns an _access_token_ and an _refresh_token_ but only in the cases where the _authorization_code_ has been given to an registered _client_ application on behalf of an known users. If the _authorization_code_ has been created based on only a registered User, no _refresh_token_ is generated.

### password flow

Using the password flow, one can get an _access_token_ without the need to first obtain an _authorization_code_ by just provider the username/password of a user.

The password flow does NOT return a _refresh_token_, only an _access_token_.

Although this flow is not really regarded as __safe__, as in essense it is as simple as a normal login by providing a username/password, it is offered especially for local automation use-cases. For example for having scripts, etc. being able to communicate with the Domoticz server without the need to send the username/password of a user with each individual request.

Instead it can provide a username/password once to obtain an _access_token_ and than use this _access_token_ till it expires. No need to provide the username/password in the meantime.

Proper scripts,etc. should behave like real _clients_ and use the _authorization_code_ flow instead.

Not implemented yet, but it probably better to limit the _password_ flow only to 'users' (scripts, etc.) that are within in _trusted network_. Otherwise the _password_ flow would also be available for use by 'users' that live outside _trusted networks_ like the Internet.

### Using _refresh_tokens_

The goal of _refresh_tokens_ is to get a new _access_token_, for example when an _access_token_ has expired, without the need of Authenticating again to get a new _authorization_code_ to exchange for a new _access_token_.

Instead by providing a (valid) _refresh_token_, a new _access_token_ and _refresh_token_ are handed out immediatly.

A _refresh_token_ also has an expiration date/time (24 hours), but that is much longer than the life-time of an _access_token_ (which is 1 hour).

The use-case is for _clients_ that keep communicating with Domoticz for a longer period of time, for example a Dashboard like Dashticz. Without a _refresh_token_, the Dasboard would require the user the re-login every hour. (Or in a _bad_ implementation, store the users credentials and re-use them each time to re-login). 

Now only at start-up, the Dashboard has to ask for the users credentials and provide these to obtain an _authorization_code_. After that the user credentials are never needed again. Unless the Dashboard has stopped working and the latest _refresh_token_ has expired. Then the user has to re-login as a safety measure.

## PKCE extension (RFC7636)

Proof Key Code Exchange (pronounced "pixy")
More secure auth_code flow for public clients (alternative to depricated implicit flow)

## OpenID Connect (OIDC) support

OpenID Connect add additional best practices and standards on top of existing standards like OAuth2 and JWT.

At this moment, no specific OIDC parts are implementend, but support for
* well-known end-point discovery
* ID Tokens

are on top of the list.


## JWT support (RFC7519)

The __access_token__ returned is a so-called _JWT_ (Json Web Token) containing all kind of relevant information and is tamper-proof because it has been digitally signed.

## More reading

https://developer.okta.com/blog/2019/10/21/illustrated-guide-to-oauth-and-oidc
https://developer.okta.com/blog/2019/05/01/is-the-oauth-implicit-flow-dead
https://blog.postman.com/pkce-oauth-how-to/


https://datatracker.ietf.org/doc/html/rfc6749
https://datatracker.ietf.org/doc/html/rfc7636
https://datatracker.ietf.org/doc/html/rfc7519

# Future developments and other options

Thanks to the support of open standards like OAuth, JWT, OIDC, etc. it becomes possible to use/integratie Domoticz into environments that use single-signon for example. Also it is possible to use external User Management for example by using an Azure AD, AWS Cognito, API Gateways or Microsoft Active Directy and Federation Services, Okta Identity or Red Hats KeyCloak (or FreeIPA), etc.

Scenario's where a central IAM system provides the User Management functionalities and hands-out the correct tokens, that than can be used by (local) Domoticz Servers. For example when managing multiple locations and installations of Domoticz.
