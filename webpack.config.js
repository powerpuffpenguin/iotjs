const path = require('path')
module.exports = {
    target: ['node', 'es5'],
    mode: 'production',
    entry: {
        main: './bin/src/main.ts',
    },
    output: {
        path: path.resolve(__dirname, 'bin'),
        filename: "bundle.js",
        libraryTarget: "commonjs",
    },
    module: {
        rules: [
            { test: /\.ts$/, use: 'ts-loader' }
        ],
    },
    externals: [
        "iotjs",
        /^iotjs\//,
    ],
    resolve: {
        modules: [
            'node_modules/'
        ],
        descriptionFiles: ['package.json'],
        extensions: ['.ts'],
    },
}