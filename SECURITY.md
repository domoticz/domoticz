# Domoticz Open Source Security Policies and Procedures

This document describes our security procedures and general policies for the Domoticz Open Source projects as found on https://github.com/domoticz.

  * [Reporting a Vulnerability](#reporting-a-vulnerability)
  * [Disclosure Policy](#disclosure-policy)
  * [Releases under maintenance](#Releases-under-maintenance)

## Reporting a Vulnerability 

The team and community of Domoticz take all security vulnerabilities seriously. Thank you for improving the security of our open source software. We appreciate your efforts and responsible disclosure and will make every effort to acknowledge your contributions.

**The preferred channel is GitHub private vulnerability reporting**, which keeps the report private to the maintainers until an advisory is published, and lets us coordinate the fix and credit you in the same place:

  * [Report a vulnerability in domoticz/domoticz](https://github.com/domoticz/domoticz/security/advisories/new)
  * [Report a vulnerability in domoticz/libwebem](https://github.com/domoticz/libwebem/security/advisories/new)

If you would rather not use GitHub, or you do not have an account, email the Domoticz team at:

    security@domoticz.com

Normally we will acknowledge your report within 24 hours, and will send a more detailed response within a week indicating the next steps in handling it. After the initial reply, the team will endeavor to keep you informed of the progress towards a fix and full announcement, and may ask for additional information or guidance.

Please report security vulnerabilities in third-party modules to the person or team maintaining the module.

### What helps us

Reports are easiest to act on when they include the affected version or commit, the steps or request needed to reproduce the issue, and what an attacker gains. If you can say what you did *not* observe, or what turned out not to be exploitable, that is genuinely useful and saves us both time. Proof-of-concept code is welcome but not required.

Please give us a reasonable opportunity to ship a fix before disclosing publicly. We will not take legal action against researchers who report in good faith and act accordingly.

## Disclosure Policy

When the team receives a security bug report, they will assign it to a primary handler. This person will coordinate the fix and release process, involving the following steps:

  * Confirm the problem and determine if any releases under maintenance are affected.
  * Audit code to find any potential similar problems.
  * Prepare fixes for all releases still under maintenance.
  * Publish a [GitHub Security Advisory](https://github.com/domoticz/domoticz/security/advisories) once fixed builds are available, crediting the reporter unless they ask otherwise.

We are happy to request a CVE as part of an advisory if you would like one. If you have already engaged a third-party CNA, tell us and we will coordinate rather than publish a competing record.

## Releases under maintenance

Currently, only the latest [_Stable_](https://www.domoticz.com/downloads/) release and the latest _Beta_ release will receive security fixes addressing reported and confirmed security issues.
