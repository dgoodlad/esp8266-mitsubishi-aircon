import { decode, encode } from './index.js';

describe("config", () => {
  let deviceName,
      ssid,
      passkey,
      hostname,
      encryption,
      dhcp,
      staticIP,
      staticDNS,
      staticGateway,
      staticSubnet;

  beforeEach(() => {
    deviceName = null,
    ssid = null,
    passkey = null,
    hostname = null,
    encryption = 0,
    dhcp = false,
    staticIP = null,
    staticDNS = null,
    staticGateway = null,
    staticSubnet = null
  });

  let toObj = function() {
    return {
      deviceName,
      ssid,
      passkey,
      hostname,
      encryption,
      dhcp,
      staticIP,
      staticDNS,
      staticGateway,
      staticSubnet
    }
  }

  let valueAt = function(index) {
    let encoded = encode(toObj());
    return parseInt(encoded.substring(index, index + 2), 16);
  }

  let charAt = function(index) {
    return String.fromCharCode(valueAt(index));
  }

  describe("encode", () => {
    describe("deviceName", () => {
      describe("is set", () => {
        beforeEach(() => { deviceName = "Test" });

        it("encodes the length", () => {
          expect(valueAt(4)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(6)).to.eq("T");
          expect(charAt(8)).to.eq("e");
          expect(charAt(10)).to.eq("s");
          expect(charAt(12)).to.eq("t");
        });
      });
    });



    describe("ssid", () => {
      describe("is set", () => {
        beforeEach(() => { ssid = "Test" });

        it("encodes the length", () => {
          expect(valueAt(6)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(8)).to.eq("T");
          expect(charAt(10)).to.eq("e");
          expect(charAt(12)).to.eq("s");
          expect(charAt(14)).to.eq("t");
        });
      });
    });

    describe("passkey", () => {
      describe("is set", () => {
        beforeEach(() => { passkey = "Test" });

        it("encodes the length", () => {
          expect(valueAt(8)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(10)).to.eq("T");
          expect(charAt(12)).to.eq("e");
          expect(charAt(14)).to.eq("s");
          expect(charAt(16)).to.eq("t");
        });
      });
    });

    describe("encryption", () => {
      describe("byte 1", () => {
        [ 0, 1, 2, 3, 4 ].forEach((v) => {
          describe(v.toString(), () => {
            beforeEach(() => { encryption = v });
            it("is " + v, () => {
              expect(valueAt(2)).to.eq(v);
            });
          });
        });
      });
    });

    describe("hostame", () => {
      describe("is set", () => {
        beforeEach(() => { hostname = "test" });

        it("encodes the length", () => {
          expect(valueAt(10)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(12)).to.eq("t");
          expect(charAt(14)).to.eq("e");
          expect(charAt(16)).to.eq("s");
          expect(charAt(18)).to.eq("t");
        });
      });
    });


    describe("dhcp", () => {
      describe("true", () => {
        beforeEach(() => { dhcp = true });
        it("is 1", () => {
          expect(valueAt(2)).to.eq(8)
        });
      });

      describe("false", () => {
        beforeEach(() => { dhcp = false });
        it("is 0", () => {
          expect(valueAt(2)).to.eq(0)
        });
      });
    });

    describe("staticIP", () => {
      describe("is set", () => {
        beforeEach(() => { staticIP = "Test" });

        it("encodes the length", () => {
          expect(valueAt(12)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(14)).to.eq("T");
          expect(charAt(16)).to.eq("e");
          expect(charAt(18)).to.eq("s");
          expect(charAt(20)).to.eq("t");
        });
      });
    });

    describe("staticDNS", () => {
      describe("is set", () => {
        beforeEach(() => { staticDNS = "Test" });

        it("encodes the length", () => {
          expect(valueAt(14)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(16)).to.eq("T");
          expect(charAt(18)).to.eq("e");
          expect(charAt(20)).to.eq("s");
          expect(charAt(22)).to.eq("t");
        });
      });
    });

    describe("staticGateway", () => {
      describe("is set", () => {
        beforeEach(() => { staticGateway = "Test" });

        it("encodes the length", () => {
          expect(valueAt(16)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(18)).to.eq("T");
          expect(charAt(20)).to.eq("e");
          expect(charAt(22)).to.eq("s");
          expect(charAt(24)).to.eq("t");
        });
      });
    });

    describe("staticSubnet", () => {
      describe("is set", () => {
        beforeEach(() => { staticSubnet = "Test" });

        it("encodes the length", () => {
          expect(valueAt(18)).to.eq(4);
        });

        it("encodes the string", () => {
          expect(charAt(20)).to.eq("T");
          expect(charAt(22)).to.eq("e");
          expect(charAt(24)).to.eq("s");
          expect(charAt(26)).to.eq("t");
        });
      });
    });
  });

  describe("decode", () => {
    let data = "00dc0546656c757808596f7572574946490b596f7572506173736b65790566656c75780c3139322e3136382e302e313007382e382e382e340b3139322e3136382e302e310d3235352e3235352e3235352e30"
    let decoded = null;
    beforeEach(() => { decoded = decode(data) });

    describe("encryption", () => {
      it("is decoded", () => {
        expect(decoded.encryption).to.eq(4);
      });
    });

    describe("dhcp", () => {
      it("is decoded", () => {
        expect(decoded.dhcp).to.eq(true);
      });
    });

    describe("deviceName", () => {
      it("is decoded", () => {
        expect(decoded.deviceName).to.eq("Felux");
      });
    });

    describe("ssid", () => {
      it("is decoded", () => {
        expect(decoded.ssid).to.eq("YourWIFI");
      });
    });

    describe("passkey", () => {
      it("is decoded", () => {
        expect(decoded.passkey).to.eq("YourPasskey");
      });
    });

    describe("hostname", () => {
      it("is decoded", () => {
        expect(decoded.hostname).to.eq("felux");
      });
    });

    describe("staticIP", () => {
      it("is decoded", () => {
        expect(decoded.staticIP).to.eq("192.168.0.10");
      });
    });

    describe("staticDNS", () => {
      it("is decoded", () => {
        expect(decoded.staticDNS).to.eq("8.8.8.4");
      });
    });

    describe("staticGateway", () => {
      it("is decoded", () => {
        expect(decoded.staticGateway).to.eq("192.168.0.1");
      });
    });

    describe("staticSubnet", () => {
      it("is decoded", () => {
        expect(decoded.staticSubnet).to.eq("255.255.255.0");
      });
    });
  });
});
