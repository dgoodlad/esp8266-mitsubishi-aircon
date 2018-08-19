import { h, Component } from 'preact';

import styles from './index.css';

export const NOT_SAVING = 0;
export const SAVING = 1;
export const RESETTING = 2;
export const SAVED = 3;

export default class Overlay extends Component {
  render() {
    let text = "";
    if(this.props.uploading == SAVING) {
      text = "Uploading…";
    } else if(this.props.uploading == RESETTING) {
      text = "Resetting…";
    } else if(this.props.uploading == SAVED) {
      text = "Updated!";
    } else {
      return "";
    }
      
    return (
      <div className={styles.container}>
        <h1 className={styles.label}>{text}</h1>
      </div>
    );
  }
}
