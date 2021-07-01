# Domoticz Security

This document describes the security options within Domoticz

By default, any Domoticz installation is open to the netwerk it is placed in. Usually that would be someone's private network at home, work, school, barn, car, etc. And for many, if not most, use-cases that is sufficient and what we want. The goal is to control things within the local environment.

So it is a __bad__ idea to connect any installation directly to the internet. Because that would mean anyone anywhere can access Domoticz and control connected devices. Could be _fun_ in some edge-cases, but most likely not what anyone would want.

The same applies for _larger_ networks, for example a school or an office, where you do not want just anyone that has access to the network, also get access to Domoticz. Schoolkids would probably love it, but teacher and staff probably not :)

## Add user-management

A first step is to activate 'User-Management'. User-Management add's a first layer of security by requiring possible users to _identify_ themselves before they get access.

Also by default, when no User-Management is active, the (anonymous) user automatically get __admin__ privileges.

## Add trusted-networks

Users coming from a _'trusted network'_ do not need to _identify_ themselves.

But when coming from 'outside' a trusted network, Domoticz will require the user to authenticate.

## OAuth2 support (RFC6749)

Only __authorization_code__ flow is supported.

The _implicit_ flow is not as it is not secure. But using _authorization_code flow with the PKCE extension_ is also possible now. PKCE stands for _Proof Key for Code Exchange_.

Maybe __client_credentials__ flow support? But only when in _trusted network_?

Support for _refresh_token_ :)


## OpenID Connect (OIDC) support

...

## JWT support (RFC7519)

The __access_token__ returned is called a _JWT_ (Json Web Token).

## More reading

https://developer.okta.com/blog/2019/10/21/illustrated-guide-to-oauth-and-oidc
https://developer.okta.com/blog/2019/05/01/is-the-oauth-implicit-flow-dead
https://blog.postman.com/pkce-oauth-how-to/
