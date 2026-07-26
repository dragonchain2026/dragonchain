var settings = require('./lib/settings');
const Client = require('bitcoin-core');
const client = new Client(settings.wallet);

console.log('Testing RPC connection...');
console.log('Settings:', settings.wallet);

client.command([{method: 'getblockcount', parameters: []}], function(err, response) {
  if (err) {
    console.error('Error:', err);
  } else {
    console.log('Response:', response);
  }
});
