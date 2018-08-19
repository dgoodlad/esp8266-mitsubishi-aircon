import { DEFAULTS } from '../config';
const KEYS = Object.keys(DEFAULTS);

export function cleanProps(props) {
  let cleaned = {};
  Object.keys(props).forEach((key) => {
    if(KEYS.indexOf(key) != -1) {
      cleaned[key] = props[key];
    }
  });
  return cleaned;
}
