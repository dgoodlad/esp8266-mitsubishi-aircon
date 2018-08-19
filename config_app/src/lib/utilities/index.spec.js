import { DEFAULTS } from '../config';
import { cleanProps } from './index';

const KEYS = Object.keys(DEFAULTS);

describe("cleanProps", () => {
  it("removes keys that aren't in the config", () => {
    let unclean = Object.assign({}, DEFAULTS, {
      unknownkey: true
    });
    expect(Object.keys(cleanProps(unclean))).to.deep.equal(KEYS);
  });
});
