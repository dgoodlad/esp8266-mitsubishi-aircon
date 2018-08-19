import { h, Component } from 'preact';

import styles from '../../modules/header.css';

export default class Header extends Component {
  render() {
    return (
      <header className={styles.header}>
        <h1>Configure Felux</h1>
      </header>
    );
  }
}
