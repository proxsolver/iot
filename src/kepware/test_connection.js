/**
 * Kepware Connection Test Script
 *
 * Run this script to test OPC-UA connection to Kepware KEPServerEX
 *
 * Usage:
 *   node src/kepware/test_connection.js
 */

const { KepwareOPCUAClient, KEPWARE_CONFIG, TAG_MAPPING } = require('./kepware_opcua_client');

async function testConnection() {
  console.log('========================================');
  console.log('Kepware OPC-UA Connection Test');
  console.log('========================================\n');

  const client = new KepwareOPCUAClient();

  try {
    // Step 1: Connect
    console.log('Step 1: Connecting to Kepware...');
    console.log(`Endpoint: ${KEPWARE_CONFIG.endpoint}`);
    console.log(`Security Mode: ${KEPWARE_CONFIG.securityMode.toString()}`);
    console.log(`Security Policy: ${KEPWARE_CONFIG.securityPolicy.toString()}\n`);

    await client.connect();
    console.log('✓ Connected successfully!\n');

    // Step 2: Test reading a tag
    console.log('Step 2: Testing tag read...');
    console.log('Attempting to read a tag...\n');

    // Try to read system status tag (should exist in Kepware)
    const testTag = TAG_MAPPING.gatewayStatus;
    console.log(`Reading tag: ${testTag}`);

    try {
      const value = await client.readTag(testTag);
      console.log('✓ Tag read successful!');
      console.log(`  Value: ${value.value}`);
      console.log(`  Quality: ${value.quality}`);
      console.log(`  Timestamp: ${value.timestamp}\n`);
    } catch (error) {
      console.log(`⚠ Tag read failed (tag may not exist yet): ${error.message}`);
      console.log('  This is normal if tags haven\'t been created in Kepware yet.\n');
    }

    // Step 3: List all configured tags
    console.log('Step 3: Configured tag mappings...');
    console.log('Make sure these tags exist in Kepware:\n');

    Object.entries(TAG_MAPPING).forEach(([name, tagPath]) => {
      console.log(`  ${name.padEnd(30)} => ${tagPath}`);
    });
    console.log('');

    // Step 4: Test writing a tag
    console.log('Step 4: Testing tag write...');
    console.log(`Writing to: ${TAG_MAPPING.gatewayStatus}`);

    try {
      const result = await client.writeTag(TAG_MAPPING.gatewayStatus, true);
      console.log('✓ Tag write successful!');
      console.log(`  Result: ${JSON.stringify(result)}\n`);
    } catch (error) {
      console.log(`⚠ Tag write failed: ${error.message}`);
      console.log('  This is normal if the tag doesn\'t exist or is read-only.\n');
    }

    // Step 5: Summary
    console.log('========================================');
    console.log('Test Summary');
    console.log('========================================\n');
    console.log('✓ OPC-UA Connection: SUCCESS');
    console.log('✓ Session Creation: SUCCESS');

    if (client.connected) {
      console.log('✓ Connection Status: CONNECTED');
    } else {
      console.log('✗ Connection Status: DISCONNECTED');
    }

    console.log('\nNext Steps:');
    console.log('1. Create tags in Kepware (see KEPWARE_SETUP_GUIDE.md)');
    console.log('2. Enable Kepware integration: export KEPWARE_ENABLED=true');
    console.log('3. Start the data pipeline: npm start\n');

    // Disconnect
    await client.disconnect();
    console.log('Disconnected from Kepware.');

  } catch (error) {
    console.error('\n✗ Connection failed!');
    console.error(`Error: ${error.message}\n`);

    console.log('Troubleshooting:');
    console.log('1. Make sure Kepware KEPServerEX is running');
    console.log('2. Verify OPC-UA server is enabled in Kepware');
    console.log('3. Check the endpoint URL is correct');
    console.log('4. Verify firewall is not blocking port 49320');
    console.log('5. Try using UAExpert client to test connection\n');

    process.exit(1);
  }
}

// Run test
testConnection().then(() => {
  process.exit(0);
}).catch((error) => {
  console.error('Unexpected error:', error);
  process.exit(1);
});
