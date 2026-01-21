#include <QtTest/QtTest>
#include "RuleGenerator.h"
#include "RuleParser.h"
#include "Types.h"

using namespace udevme;

class TestRules : public QObject {
    Q_OBJECT

private slots:
    void testRuleGeneration();
    void testRuleGenerationDeterminism();
    void testParseRoundTrip();
    void testMetadataComment();
    void testPermissionLevelConversion();
    void testHashComputation();
};

void TestRules::testRuleGeneration() {
    UdevRule rule;
    rule.id = QUuid::fromString("12345678-1234-1234-1234-123456789abc");
    
    DeviceInfo dev;
    dev.vendorId = "1234";
    dev.productId = "5678";
    dev.name = "Test Device";
    dev.hasHidraw = true;
    rule.devices.append(dev);
    
    rule.permissionLevel = PermissionLevel::Safe;
    rule.ruleTypes.hidraw = true;
    rule.ruleTypes.uaccess = true;
    rule.enabled = true;
    
    QString generated = RuleGenerator::generateSingleRule(rule);
    
    QVERIFY(!generated.isEmpty());
    QVERIFY(generated.contains("udevme:"));
    QVERIFY(generated.contains("id=12345678-1234-1234-1234-123456789abc"));
    QVERIFY(generated.contains("KERNEL==\"hidraw*\""));
    QVERIFY(generated.contains("ATTRS{idVendor}==\"1234\""));
    QVERIFY(generated.contains("ATTRS{idProduct}==\"5678\""));
    QVERIFY(generated.contains("TAG+=\"uaccess\""));
}

void TestRules::testRuleGenerationDeterminism() {
    UdevRule rule;
    rule.id = QUuid::fromString("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    
    DeviceInfo dev;
    dev.vendorId = "abcd";
    dev.productId = "ef01";
    rule.devices.append(dev);
    
    rule.permissionLevel = PermissionLevel::Balanced;
    rule.ruleTypes.hidraw = true;
    rule.enabled = true;
    
    // Generate multiple times
    QString gen1 = RuleGenerator::generateSingleRule(rule);
    QString gen2 = RuleGenerator::generateSingleRule(rule);
    QString gen3 = RuleGenerator::generateSingleRule(rule);
    
    // All should be identical
    QCOMPARE(gen1, gen2);
    QCOMPARE(gen2, gen3);
}

void TestRules::testParseRoundTrip() {
    // Create a rule
    UdevRule original;
    original.id = QUuid::fromString("11111111-2222-3333-4444-555555555555");
    
    DeviceInfo dev1;
    dev1.vendorId = "1111";
    dev1.productId = "2222";
    dev1.name = "Device One";
    dev1.hasHidraw = true;
    original.devices.append(dev1);
    
    DeviceInfo dev2;
    dev2.vendorId = "3333";
    dev2.productId = "4444";
    dev2.name = "Device Two";
    original.devices.append(dev2);
    
    AppInfo app;
    app.desktopId = "brave-browser.desktop";
    app.name = "Brave";
    original.applications.append(app);
    
    original.permissionLevel = PermissionLevel::Open;
    original.ruleTypes.hidraw = true;
    original.ruleTypes.usb = true;
    original.enabled = true;
    
    // Generate rules file
    QVector<UdevRule> rules;
    rules.append(original);
    QString generated = RuleGenerator::generateRulesFile(rules);
    
    // Parse it back
    auto parseResult = RuleParser::parseRulesFile(generated);
    
    QVERIFY(parseResult.success);
    QCOMPARE(parseResult.rules.size(), 1);
    
    const UdevRule& parsed = parseResult.rules[0];
    
    QCOMPARE(parsed.id, original.id);
    QCOMPARE(parsed.devices.size(), original.devices.size());
    QCOMPARE(parsed.permissionLevel, original.permissionLevel);
    QCOMPARE(parsed.ruleTypes.hidraw, original.ruleTypes.hidraw);
    QCOMPARE(parsed.ruleTypes.usb, original.ruleTypes.usb);
    
    // Regenerate from parsed and compare
    QVector<UdevRule> parsedRules;
    parsedRules.append(parsed);
    QString regenerated = RuleGenerator::generateRulesFile(parsedRules);
    
    // The regenerated content should be identical
    QCOMPARE(regenerated, generated);
}

void TestRules::testMetadataComment() {
    UdevRule rule;
    rule.id = QUuid::fromString("deadbeef-dead-beef-dead-beefdeadbeef");
    
    DeviceInfo dev;
    dev.vendorId = "cafe";
    dev.productId = "babe";
    rule.devices.append(dev);
    
    AppInfo app1, app2;
    app1.desktopId = "app1.desktop";
    app2.desktopId = "app2.desktop";
    rule.applications.append(app1);
    rule.applications.append(app2);
    
    rule.permissionLevel = PermissionLevel::Safe;
    rule.ruleTypes.hidraw = true;
    rule.ruleTypes.uaccess = true;
    rule.enabled = true;
    
    QString comment = RuleGenerator::generateMetadataComment(rule);
    
    QVERIFY(comment.startsWith("# udevme:"));
    QVERIFY(comment.contains("id=deadbeef-dead-beef-dead-beefdeadbeef"));
    QVERIFY(comment.contains("devices=cafe:babe"));
    QVERIFY(comment.contains("apps=app1.desktop,app2.desktop"));
    QVERIFY(comment.contains("level=Safe"));
    QVERIFY(comment.contains("enabled=true"));
}

void TestRules::testPermissionLevelConversion() {
    QCOMPARE(permissionLevelToString(PermissionLevel::Safe), QString("Safe"));
    QCOMPARE(permissionLevelToString(PermissionLevel::Balanced), QString("Balanced"));
    QCOMPARE(permissionLevelToString(PermissionLevel::Open), QString("Open"));
    
    QCOMPARE(permissionLevelFromString("Safe"), PermissionLevel::Safe);
    QCOMPARE(permissionLevelFromString("Balanced"), PermissionLevel::Balanced);
    QCOMPARE(permissionLevelFromString("Open"), PermissionLevel::Open);
    QCOMPARE(permissionLevelFromString("Invalid"), PermissionLevel::Safe); // Default
}

void TestRules::testHashComputation() {
    QString content1 = "test content";
    QString content2 = "test content";
    QString content3 = "different content";
    
    QString hash1 = RuleParser::computeHash(content1);
    QString hash2 = RuleParser::computeHash(content2);
    QString hash3 = RuleParser::computeHash(content3);
    
    QVERIFY(!hash1.isEmpty());
    QCOMPARE(hash1, hash2); // Same content = same hash
    QVERIFY(hash1 != hash3); // Different content = different hash
    QCOMPARE(hash1.length(), 64); // SHA256 hex = 64 chars
}

QTEST_MAIN(TestRules)
#include "test_rules.moc"
