#
# This file is part of pysnmp software.
#
# Copyright (c) 2005-2017, Ilya Etingof <etingof@gmail.com>
# License: http://pysnmp.sf.net/license.html
#


class ErrorIndication(Exception):
    """SNMPv3 error-indication values"""

    def __init__(self, descr=None):
        self.__value = self.__descr = self.__class__.__name__[0].lower() + self.__class__.__name__[1:]
        if descr:
            self.__descr = descr

    def __eq__(self, other):
        return self.__value == other

    def __ne__(self, other):
        return self.__value != other

    def __lt__(self, other):
        return self.__value < other

    def __le__(self, other):
        return self.__value <= other

    def __gt__(self, other):
        return self.__value > other

    def __ge__(self, other):
        return self.__value >= other

    def __str__(self):
        return self.__descr


# SNMP message processing errors

class SerializationError(ErrorIndication):
    pass

serializationError = SerializationError('SNMP message serialization error')


class DeserializationError(ErrorIndication):
    pass

deserializationError = DeserializationError('SNMP message deserialization error')


class ParseError(DeserializationError):
    pass

parseError = ParseError('SNMP message deserialization error')


class UnsupportedMsgProcessingModel(ErrorIndication):
    pass

unsupportedMsgProcessingModel = UnsupportedMsgProcessingModel('Unknown SNMP message processing model ID encountered')


class UnknownPDUHandler(ErrorIndication):
    pass

unknownPDUHandler = UnknownPDUHandler('Unhandled PDU type encountered')


class UnsupportedPDUtype(ErrorIndication):
    pass

unsupportedPDUtype = UnsupportedPDUtype('Unsupported SNMP PDU type encountered')


class RequestTimedOut(ErrorIndication):
    pass

requestTimedOut = RequestTimedOut('No SNMP response received before timeout')


class EmptyResponse(ErrorIndication):
    pass

emptyResponse = EmptyResponse('Empty SNMP response message')


class NonReportable(ErrorIndication):
    pass

nonReportable = NonReportable('Report PDU generation not attempted')


class DataMismatch(ErrorIndication):
    pass

dataMismatch = DataMismatch('SNMP request/response parameters mismatched')


class EngineIDMismatch(ErrorIndication):
    pass

engineIDMismatch = EngineIDMismatch('SNMP engine ID mismatch encountered')


class UnknownEngineID(ErrorIndication):
    pass

unknownEngineID = UnknownEngineID('Unknown SNMP engine ID encountered')


class TooBig(ErrorIndication):
    pass

tooBig = TooBig('SNMP message will be too big')


class LoopTerminated(ErrorIndication):
    pass

loopTerminated = LoopTerminated('Infinite SNMP entities talk terminated')


class InvalidMsg(ErrorIndication):
    pass

invalidMsg = InvalidMsg('Invalid SNMP message header parameters encountered')


# SNMP security modules errors

class UnknownCommunityName(ErrorIndication):
    pass

unknownCommunityName = UnknownCommunityName('Unknown SNMP community name encountered')


class NoEncryption(ErrorIndication):
    pass

noEncryption = NoEncryption('No encryption services configured')


class EncryptionError(ErrorIndication):
    pass

encryptionError = EncryptionError('Ciphering services not available')


class DecryptionError(ErrorIndication):
    pass

decryptionError = DecryptionError('Ciphering services not available or ciphertext is broken')


class NoAuthentication(ErrorIndication):
    pass

noAuthentication = NoAuthentication('No authentication services configured')


class AuthenticationError(ErrorIndication):
    pass

authenticationError = AuthenticationError('Ciphering services not available or bad parameters')


class AuthenticationFailure(ErrorIndication):
    pass

authenticationFailure = AuthenticationFailure('Authenticator mismatched')


class UnsupportedAuthProtocol(ErrorIndication):
    pass

unsupportedAuthProtocol = UnsupportedAuthProtocol('Authentication protocol is not supprted')


class UnsupportedPrivProtocol(ErrorIndication):
    pass

unsupportedPrivProtocol = UnsupportedPrivProtocol('Privacy protocol is not supprted')


class UnknownSecurityName(ErrorIndication):
    pass

unknownSecurityName = UnknownSecurityName('Unknown SNMP security name encountered')


class UnsupportedSecurityModel(ErrorIndication):
    pass

unsupportedSecurityModel = UnsupportedSecurityModel('Unsupported SNMP security model')


class UnsupportedSecurityLevel(ErrorIndication):
    pass

# backward compatibility plug
UnsupportedSecLevel = UnsupportedSecurityLevel

unsupportedSecurityLevel = UnsupportedSecurityLevel('Unsupported SNMP security level')


class NotInTimeWindow(ErrorIndication):
    pass

notInTimeWindow = NotInTimeWindow('SNMP message timing parameters not in windows of trust')


class UnknownUserName(ErrorIndication):
    pass

unknownUserName = UnknownUserName('Unknown USM user')


class WrongDigest(ErrorIndication):
    pass

wrongDigest = WrongDigest('Wrong SNMP PDU digest')


class ReportPduReceived(ErrorIndication):
    pass

reportPduReceived = ReportPduReceived('Remote SNMP engine reported error')


# SNMP access-control errors

class NoSuchView(ErrorIndication):
    pass

noSuchView = NoSuchView('No such MIB view currently exists')


class NoAccessEntry(ErrorIndication):
    pass

noAccessEntry = NoAccessEntry('Access to MIB node denined')


class NoGroupName(ErrorIndication):
    pass

noGroupName = NoGroupName('No such VACM group configured')


class NoSuchContext(ErrorIndication):
    pass

noSuchContext = NoSuchContext('SNMP context now found')


class NotInView(ErrorIndication):
    pass

notInView = NotInView('Requested OID is out of MIB view')


class AccessAllowed(ErrorIndication):
    pass

accessAllowed = AccessAllowed()


class OtherError(ErrorIndication):
    pass

otherError = OtherError('Unspecified SNMP engine error occurred')


# SNMP Apps errors

class OidNotIncreasing(ErrorIndication):
    pass

oidNotIncreasing = OidNotIncreasing('OID not increasing')
