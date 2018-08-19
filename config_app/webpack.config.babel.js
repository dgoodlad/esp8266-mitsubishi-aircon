import webpack from 'webpack';
import ExtractTextPlugin from 'extract-text-webpack-plugin';
import CssoWebpackPlugin from 'csso-webpack-plugin';
import HtmlWebpackPlugin from 'html-webpack-plugin';
import HtmlWebpackInlineSourcePlugin from 'html-webpack-inline-source-plugin';
import ReplacePlugin from 'replace-bundle-webpack-plugin';
import fileUpload from 'express-fileupload';
import path from 'path';
import fs from 'fs';
const ENV = process.env.NODE_ENV || 'development';

module.exports = {
	context: path.resolve(__dirname, "src"),
	entry: './index.js',

	output: {
		path: path.resolve(__dirname, "build"),
		publicPath: '/',
		filename: 'bundle.js'
	},

	resolve: {
		extensions: ['.jsx', '.js' ],
		modules: [
			path.resolve(__dirname, "src/lib"),
			path.resolve(__dirname, "node_modules"),
			'node_modules'
		]
	},

	module: {
		loaders: [
			{
				test: /\.jsx?$/,
				exclude: /node_modules/,
				loader: 'babel-loader'
			},
      {
        test: /\.css$/,
        use: ExtractTextPlugin.extract({
          fallback: "style-loader",
          use: "css-loader?modules&localIdentName=" + (ENV == 'production' ? "[hash:base64:4]" : "[name]__[local]___[hash:base64:5]")
        })
      },
			{
				test: /\.(xml|html|txt|md)$/,
				loader: 'raw-loader'
			}
		]
	},

	plugins: ([
		new webpack.NoEmitOnErrorsPlugin(),
		new webpack.DefinePlugin({
			'process.env.NODE_ENV': JSON.stringify(ENV)
		}),
    new ExtractTextPlugin({ filename: 'style.css', allChunks: true, disable: ENV !== 'production' }),
		new HtmlWebpackPlugin({
			template: './index.ejs',
			minify: { collapseWhitespace: true },
      inlineSource: '(.js|.css)$'
		}),
    new HtmlWebpackInlineSourcePlugin()
	]).concat(ENV === 'production' ? [
    new CssoWebpackPlugin({
      sourceMap: false
    }),
		new webpack.optimize.UglifyJsPlugin({
      mangle: true,
			output: {
				comments: false
			},
			compress: {
				warnings: false,
				conditionals: true,
				unused: true,
				comparisons: true,
				sequences: true,
				dead_code: true,
        drop_console: true,
        unsafe: true,
				evaluate: true,
				if_return: true,
				join_vars: true,
        collapse_vars: true,
        reduce_vars: true,
				negate_iife: false
			}
		}),
		// strip out babel-helper invariant checks
		new ReplacePlugin([{
			// this is actually the property name https://github.com/kimhou/replace-bundle-webpack-plugin/issues/1
			partten: /throw\s+(new\s+)?[a-zA-Z]+Error\s*\(/g,
			replacement: () => 'return;('
		}])
	] : []),


	stats: { colors: true },

	node: {
		global: true,
		process: false,
		Buffer: false,
		__filename: false,
		__dirname: false,
		setImmediate: false
	},

	devtool: ENV === 'production' ? false : 'cheap-module-eval-source-map',

	devServer: {
		port: process.env.PORT || 8080,
		host: 'localhost',
		publicPath: '/',
		contentBase: './src',
		historyApiFallback: true,
		open: true,
		setup(app) {
      var delayPing = false;

      app.use(fileUpload());
			app.get("/config/ping", function(req, res) {
        if(delayPing) {
          setTimeout(function() {
            delayPing = false;
          }, 5000);
        } else {
          res.writeHead(200, { 'Content-type': 'text/plain' });
          res.end("pong");
        }
      });

      app.get("/config.json", function(req, res) {
				fs.readFile('data/config.json', 'utf-8', function(error, data) {
					if(error) {
            res.status(404).send("Not Found");
          } else {
            res.writeHead(200, { 'Content-type': 'text/json' });
					  res.end(data);
          }
				});
			});

			app.post("/config.json", function(req, res) {
        delayPing = true;
			});

			app.get("/aps.json", function(req, res) {
				res.json([
					{"ssid":"OPTUSVD3C49EE8","rssi":-90,"encryption":"8"},
					{"ssid":"Dean&Carol.b","rssi":-92,"encryption":"7"},
					{"ssid":"NETGEAR19","rssi":-76,"encryption":"4"},
					{"ssid":"MyWifi","rssi":-60,"encryption":"4"},
					{"ssid":"OPTUS_B8EC72","rssi":-89,"encryption":"8"},
					{"ssid":"OPTUS_A49184","rssi":-85,"encryption":"8"}
				]);
			});

      app.post("/firmware", function(req, res) {
        if (!req.files) {
          return res.status(400).send('No files were uploaded.');
        }
        delayPing = true; 
      });
		}
	}
};
