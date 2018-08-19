import { h, render } from 'preact';

let root;
function init() {
	let App = require('./components/app').default;
	root = render(<App />, document.body, root);
}

// in development, set up HMR:
if (module.hot) {
	//require('preact/devtools');   // turn this on if you want to enable React DevTools!
	module.hot.accept('./components/app', () => requestAnimationFrame(init) );
}

init();
