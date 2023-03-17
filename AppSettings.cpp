//
// Created by andrei on 16.03.23.
//

#include "AppSettings.h"

static const std::string CA_CERTIFICATE =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDpzCCAo+gAwIBAgIJANvEtMU8P4w8MA0GCSqGSIb3DQEBCwUAMGoxCzAJBgNV\n"
        "BAYTAlJVMQ4wDAYDVQQIDAVTYWtoYTEeMBwGA1UECgwVYXZ0b3Zva3phbC15YWt1\n"
        "dHNrLnJ1MQswCQYDVQQLDAJDQTEeMBwGA1UEAwwVYXZ0b3Zva3phbC15YWt1dHNr\n"
        "LnJ1MB4XDTE3MDIxNzA0MzQwMVoXDTI3MDIxNTA0MzQwMVowajELMAkGA1UEBhMC\n"
        "UlUxDjAMBgNVBAgMBVNha2hhMR4wHAYDVQQKDBVhdnRvdm9remFsLXlha3V0c2su\n"
        "cnUxCzAJBgNVBAsMAkNBMR4wHAYDVQQDDBVhdnRvdm9remFsLXlha3V0c2sucnUw\n"
        "ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCh3z9PPtT2itGTXcVCLkSq\n"
        "GBfXZgZt+ioiWbXlCjkaSb6rj0dZ6ConLUpp7WGYKvtzJYUr7mm8C+nKwwfPFLaT\n"
        "OzDVM4Uo6u3HVsyuA5p/t7M/ROJeLWUC4dmUXM7wXdIVLC+p0jJNcwPPZARf+KFN\n"
        "8rvrKOc07HRDohOm8LFPAh2OmopCi1xME3OcPak/64JIbRK2GjD0p6d5FrjJBrrl\n"
        "GgBlCZ2cWo49Ouo1jeGD+ynA9sk1a7o8IT8qOUB0AIn4HmuqAzQrLTAVqrZphoXS\n"
        "jqESx7f/hJtY4o3bqv0anQZnjG/v+DDEW21sz7Hv6l1tg680V+llkrRQ/GZLjwn3\n"
        "AgMBAAGjUDBOMB0GA1UdDgQWBBRlOx1xf2mGijBQPBCn4HxQk4TyTTAfBgNVHSME\n"
        "GDAWgBRlOx1xf2mGijBQPBCn4HxQk4TyTTAMBgNVHRMEBTADAQH/MA0GCSqGSIb3\n"
        "DQEBCwUAA4IBAQCEaA6syuhtXUh13mbJ99UCbPgxYc3eIvxF81Qz2A5euc6FLCqa\n"
        "Tln2daSRRvZfuexvi+O1rnEGkYTtsbnWOr7tYMQWVa0F4VdC1EzbtnhnlsoFzqvJ\n"
        "B9PJ6BzXdGuhrgZ2Xsb2kjzP+pnX5KFTp24tyjPKmEyQDxRRw0cy/FLxI2AaeXiZ\n"
        "QUYblQZAjeKJh3l37FLQUeS4RN6vmAHLTC8Xt6NPfR0C6R6ZzmrF0crOSiAQcxGT\n"
        "wRgvBlDimOc9Kaj9L+8S7/C3xfWyqQPJeWhLUyY0VSw912VTItrEsdXHTjMN9dLf\n"
        "h/PI3UgVqhhy51GUnIRokI/1VDZhmlQpoW8K\n"
        "-----END CERTIFICATE-----";

static const std::string KEY_SERVER =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIICWwIBAAKBgQDfyyd9nd5wARYV+8+JVIMBTUDq3h8l58DFBy11q4bE7viPx4hN\n"
        "qrBkFRuln71l0DJES4uSGMsLebesf8JSfC6GtTb32169gwF+CxJEMuF7d+osqxls\n"
        "95T0h5GEeK2N14b0YM+gO4bZRK3BCDhrou57n5AWyMPf/59t5nTmdQ47AQIDAQAB\n"
        "AoGAaiNFpXP8DQ7W1LJKqY0lXLaw9TNHfoi9ijRvQQGKt1fHFxnA8WTkp3LDW/e4\n"
        "sdLeJDnYBgAwPE8L54noNYqj3WhwGCxrPr1ctjkijoCfHdPTTJQ2gvyHmRjf1HzZ\n"
        "5d6ddYS9AUg/hL/f2i0cexvgH6n9VVSSKo/rGmXZZ3KKE+ECQQD2EM46IEQIDIyg\n"
        "CPBnvpNwE2YvxF6ZtCXX/uCNRjNyGB9kp3cwzduuKvMUFu4Ct+xV+nXECXZWOupM\n"
        "fi45Y70lAkEA6NQoPe+Pus8Xr2QghW15U8jegLW1Cz87K5IRq6sQ28b981sxL3uW\n"
        "0hZVEfDUOBiuqq6XKy3kevnJKD0lSCf1rQJAdf77Fc6RMRbrfHas6eLuiskSsIdH\n"
        "IgfMOfbEMZrfYrAEpdzUgDfHR47a/+A7BFy0XUp8nKQ49gLMhh/gBK86bQJBAIpS\n"
        "WKILK9Go7QJXMvbyBe2f3YiL60YGR8nn2rWpSLe5LfAhEkSKYRzpBZtF/15gU4y1\n"
        "crvFZQMmWLqeMQMAMnUCPzjr++Za+d5zog69faFt7ESMcGl9BFk6k7E7c8fzQSJD\n"
        "cSrKW7jqJMSyTfv7Xds/1o0ZUxiNK4X3je1yMjkBew==\n"
        "-----END RSA PRIVATE KEY-----";

static const std::string CERTIFICATE_SERVER =
        "-----BEGIN CERTIFICATE-----\n"
        "MIICyjCCAbICAQIwDQYJKoZIhvcNAQELBQAwajELMAkGA1UEBhMCUlUxDjAMBgNV\n"
        "BAgMBVNha2hhMR4wHAYDVQQKDBVhdnRvdm9remFsLXlha3V0c2sucnUxCzAJBgNV\n"
        "BAsMAkNBMR4wHAYDVQQDDBVhdnRvdm9remFsLXlha3V0c2sucnUwHhcNMTcwMjE3\n"
        "MDQzNTQzWhcNMjcwMjE1MDQzNTQzWjBwMQswCQYDVQQGEwJSVTEOMAwGA1UECAwF\n"
        "U2FraGExHjAcBgNVBAoMFWF2dG92b2t6YWwteWFrdXRzay5ydTEPMA0GA1UECwwG\n"
        "c2VydmVyMSAwHgYDVQQDDBcqLmF2dG92b2t6YWwteWFrdXRzay5ydTCBnzANBgkq\n"
        "hkiG9w0BAQEFAAOBjQAwgYkCgYEA38snfZ3ecAEWFfvPiVSDAU1A6t4fJefAxQct\n"
        "dauGxO74j8eITaqwZBUbpZ+9ZdAyREuLkhjLC3m3rH/CUnwuhrU299tevYMBfgsS\n"
        "RDLhe3fqLKsZbPeU9IeRhHitjdeG9GDPoDuG2UStwQg4a6Lue5+QFsjD3/+fbeZ0\n"
        "5nUOOwECAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAV24tT+teLvKnA5FQmk23sd2h\n"
        "0VQByvfUCxJPjeQZjMJSjVtXTQVEu5F1pDNCpXx3yuZi7HBGE/RNg6pbJBkKsi11\n"
        "iVFLyHM0lh2QaeXWOxBuD/+EEUoR1U2iA2sgBds1vI3sbW8SjN0ZaI7vJW55mGld\n"
        "2lgXHbDgHdaQzFRHFMvz6mRUDvhvHQJi3o/PZ3iyhU+NW7ROYJiUF0KWp7dTHpXU\n"
        "xl/YCDER/OmxTtXzPvldAptsnldGQEPtH8rCbFxq29ME9R+OspRO59m17cO4gUc+\n"
        "uzRW4hKPEyUDNpD7NKWLXcmlzOs1YN8T+1XhVus/slfTN9rKx5N3XW3G2azwpw==\n"
        "-----END CERTIFICATE-----";

const std::string& AppSettings::certificate_ca()
{
    return CA_CERTIFICATE;
}

const std::string& AppSettings::certificate_server()
{
    return CERTIFICATE_SERVER;
}

const std::string& AppSettings::key_server()
{
    return KEY_SERVER;
}
