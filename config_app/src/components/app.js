import { h, Component } from 'preact';

import Header from './header';
import Overlay from './overlay';
import { NOT_SAVING, SAVING, RESETTING, SAVED } from './overlay';
import Tab from 'configduino/components/tab';
import Firmware from 'configduino/components/firmware';
import WifiPanel from 'configduino/components/wifi-panel';
import NetworkPanel from 'configduino/components/network-panel';
import MqttPanel from 'configduino/components/mqtt-panel';
import Button from 'configduino/components/button';

import Input from 'configduino/components/input';
import * as Validation from 'configduino/validation/validator.js';

import Form from 'configduino/validation/form';
import { cleanProps } from '../lib/utilities/index.js';

import { encode, decode, DEFAULTS as CONFIG_DEFAULTS } from '../lib/config';

const TAB_SETTINGS = 0;
const TAB_FIRMWARE = 1;

import styles from './app.css';

export default class App extends Component {
  constructor() {
    super();

    this.state = Object.assign({}, CONFIG_DEFAULTS, {
      scan: true,
      tab: TAB_SETTINGS,
      loaded: false,
      uploading: NOT_SAVING
    });

    this.fetchConfig();
  }

  ping() {
    return window.fetch("/config/ping").then((response) => {
      if(!response.ok) {
        throw Error();
      }
      return response.text();
    });
  }

  fetchConfig() {
    window.fetch("/config.dat").then((response) => {
      if(!response.ok) {
        throw Error(response.statusText);
      }
      return response.text();
    }).then((config) => {
      let decoded = decode(config);
      decoded.scan = false;
      decoded.loaded = true;
      this.setState(decoded);
    }).catch((error) => {
      // Treat an error as a missing config.
    });
  }

  waitForReset(e) {
    // Uploading firmware will result in the device resetting, which will actually just kill the connection
    // so there will never be a success. On a connection event, start pinging - once the server is back up, we can re-request
    // the config file, and start again.
    this.setState({ uploading: RESETTING });
    this.ping().then(() => {
      this.setState({ uploading: SAVED });
      this.fetchConfig();
      setTimeout(() => {
        this.setState({ uploading: NOT_SAVING }, 1000);
      });
    }).catch(() => {
      setTimeout(() => {
        this.waitForReset(e);
      }, 1000);
    });
  }

  saveConfig(e) {
    e.preventDefault();
    console.log(this.state);
    let formData = new FormData();
    formData.append("config.json", encode(this.state));

    let files = e.target.querySelectorAll("input[type=\"file\"]")
    for(let i = 0; i < files.length; i++) {
      let el = files[i];
      let filename = el.getAttribute("data-filename");
      if(filename && el.files.length > 0) {
        formData.append(filename, el.files[0]);
      }
    }
    
    this.setState({ uploading: SAVING });
    let xhr = new XMLHttpRequest();
    xhr.addEventListener("error", this.waitForReset.bind(this));
    xhr.open("post", "/config/update");
    xhr.send(formData);
  }

  uploadFirmware(e) {
    e.preventDefault();
    
    this.setState({ uploading: SAVING });

    let xhr = new XMLHttpRequest();
    xhr.addEventListener("error", this.waitForReset.bind(this));
    xhr.open("post", "/firmware");
    xhr.send(new FormData(e.target));
  }

  update(settings) {
    let cleaned = cleanProps(settings);
    if(typeof(settings.scan) != "undefined") {
      cleaned.scan = settings.scan;
    }
    this.setState(cleaned);
  }

  changeTab(tab) {
    return((e) => {
      e.preventDefault();
      this.setState({ tab: tab });
    });
  }
  
  changeDeviceName(e) {
    this.setState({ deviceName: e.target.value });
  }

  render() {
    return (
      <div id="app" className={styles.container}>
        <Header />
        <Overlay uploading={this.state.uploading} />

        <nav>
          <ul className={styles.tabs}>
            <li><a href="#" className={this.state.tab == TAB_SETTINGS ? styles['current-tab'] : styles.tab} onClick={this.changeTab(TAB_SETTINGS).bind(this)}>Settings</a></li>
            <li><a className={this.state.tab == TAB_FIRMWARE ? styles['current-tab'] : styles.tab} href="#" onClick={this.changeTab(TAB_FIRMWARE).bind(this)}>Firmware</a></li>
          </ul>
        </nav>

        <Tab name={TAB_SETTINGS} current={this.state.tab}>
          <Form class={styles.form} onSubmit={this.saveConfig.bind(this)}>
            <section className={styles.panel}>
              <h3 className={styles.heading}>ESP-AC Settings</h3>
              
              <Input
                label="Room Name"
                type="text"
                value={this.state.deviceName}
                autocomplete="off"
                autocapitalize="off"
                onInput={this.changeDeviceName.bind(this)}
                validators={[ Validation.required(), Validation.length(255) ]}
                />

            </section>

            <WifiPanel
              {...this.state}
              onUpdate={this.update.bind(this)}
              />

            <NetworkPanel
              {...this.state}
              onUpdate={this.update.bind(this)}
              />

            <MqttPanel
              {...this.state}
              onUpdate={this.update.bind(this)}
              />

            <Button>Save</Button>
          </Form>
        </Tab>

        <Tab name={TAB_FIRMWARE} current={this.state.tab}>
          <Form class={styles.form} onSubmit={this.uploadFirmware.bind(this)}>
            <Firmware />
            <Button>Upload</Button>
          </Form>
        </Tab>
      </div>
    )
  }
}
