import App from './app.js';
import fs from 'fs';

import { DEFAULTS } from '../lib/config';
const KEYS = Object.keys(DEFAULTS);

describe("<App /", () => {
  describe("fetchConfig", () => {
    let config = {};

    describe("config exists", () => {
      let _subject = null;
      let subject = () => {
        if(_subject == null) {
          _subject = new App();
        }
        return _subject;
      };
      let fetchStub, response;

      beforeEach(() => {
        response = {
          ok: true,
          text: () => {
            return config
          }
        }
        fetchStub = sinon.stub(window, 'fetch').resolves(response);
        subject().fetchConfig();
        fetchStub();
      });

      afterEach(() => {
        fetchStub.restore();
      });

      it("sets scan to false", (done) => {
        setTimeout(() => {
          expect(subject().state.scan).to.equal(false);
          done();
        });
      });

      it("sets loaded to true", () => {
        setTimeout(() => {
          expect(subject().state.loaded).to.equal(true);
          done();
        });
      });

      it("decodes the config file", () => {
        setTimeout(() => {
          expect(subject().state.deviceName).to.equal("garage");
          done();
        });
      });
    });

    describe("config does not exists", () => {
      let _subject = null;
      let subject = () => {
        if(_subject == null) {
          _subject = new App();
        }
        return _subject;
      };
      let fetchStub, response;

      beforeEach(() => {
        response = {
          ok: false,
          statusText: "Not found"
        }
        fetchStub = sinon.stub(window, 'fetch').resolves(response);
        subject().fetchConfig();
        fetchStub();
      });

      afterEach(() => {
        fetchStub.restore();
      });

      it("does nothing", (done) => {
        setTimeout(() => {
          expect(subject().state.loaded).to.equal(false);
          done();
        });
      });
    });
  });

  describe("saveConfig()", () => {
    
  });

  describe("uploadFirmware()", () => {
  
  });
  
  describe("update()", () => {
    let stub, args, cleaned, app;

    beforeEach(() => {
      args = {};
      stub = sinon.stub();
      stub.returns(cleaned);
      app = new App();
    });

    it("cleans the settings", () => {
      args = Object.assign({}, DEFAULTS, {
        unknownkey: true
      });
      app.state = {};
      app.update(args);
      expect(Object.keys(app.state)).to.deep.equal(KEYS);
    });

    it("adds the scan setting is it is set in the original object", () => {
      args = Object.assign({}, DEFAULTS, {
        scan: true
      });
      app.state = {};
      app.update(args);
      expect(app.state.scan).to.equal(true);
    });
  });

  describe("changeTab()", () => {
    let app, closure, preventDefault;

    beforeEach(() => {
      app = new App();
      closure = app.changeTab('test');
      preventDefault = sinon.stub();
      closure({ preventDefault: preventDefault });
    });

    it("calls preventDefault", () => {
      expect(preventDefault).to.have.been.called;
    });

    it("sets the state value", () => {
      expect(app.state.tab).to.equal('test');
    });
  });

  describe("render()", () => {
    
  });
});
