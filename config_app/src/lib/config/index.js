// Order is very important here
export const DEFAULTS = {
  encryption: '7',
  dhcp: true,
 
  deviceName: '',
  ssid: '',
  passkey: '',
  hostname: '',
  staticIP: '',
  staticDNS: '',
  staticGateway: '',
  staticSubnet: '',
}
const STRINGS = Object.keys(DEFAULTS).slice(2);

function deserializeString(buffer, offset) {
  let len = buffer[offset];
  let start = offset + 1;
  let end = start + len;


  let string = buffer.slice(start, end).map((hex) => { return String.fromCharCode(hex) }).join('');
  return [ end, string ];
}

function serializeString(buffer, str) {
  if(typeof(str) != "string") {
    str = "";
  }
  let encoded = [ str.length ].concat(str.split('').map((chr) => chr.charCodeAt(0)));
  return buffer.concat(encoded);
}

export function encode(obj) {
  let bytes = [ 0, 0 ];

  bytes[1] = obj.encryption & 0x07;
  bytes[1] += obj.dhcp ? 8 : 0;

  // Generator that serializes all the strings
  STRINGS.forEach((key) => {
    bytes = serializeString(bytes, obj[key]);
  });

  let hex = '';
  bytes.forEach((byte) => {
    let h = byte.toString(16);
    if(h.length == 1) {
      h = "0" + h;
    }
    hex += h;
  });

  return hex;
}

export function decode(str) {
  let bytes = [];
  for(let i = 0; i < str.length / 2; i++) {
    bytes.push(parseInt(str.substring(i * 2, (i * 2) + 2), 16));
  }

  let obj = Object.assign({}, DEFAULTS, {
    encryption: bytes[1] & 0x07,
    dhcp: ((bytes[1] >> 3) & 0x01) == 1,
  });

  let offset = 2;

  // Generator that deserializes all the strings
  STRINGS.forEach((key) => {
    [ offset, obj[key] ] = deserializeString(bytes, offset);
  });

  return obj;
}
