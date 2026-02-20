#include <QtTest/QtTest>

class TestSmoke : public QObject
{
    Q_OBJECT

private slots:
    void test_true() { QVERIFY(true); }
};

QTEST_MAIN(TestSmoke)
#include "test_smoke.moc"
