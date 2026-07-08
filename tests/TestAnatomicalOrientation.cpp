#include "qtviewerpro/core/AnatomicalOrientation.h"

#include <QtTest/QtTest>

class TestAnatomicalOrientation : public QObject
{
  Q_OBJECT

private slots:
  void parsesLps();
  void parsesRai();
  void parsesRip();
  void rejectsUnknownPlaceholder();
  void rejectsEmptyString();
  void rejectsPartialUnknown();
  void rejectsInvalidLetters();
  void rejectsDuplicateAnatomicalFamily();
  void rejectsLowercaseAcronym();
};

void TestAnatomicalOrientation::parsesLps()
{
  const auto anatomy = qvp::parseAnatomicalOrientationAcronym("LPS");

  QVERIFY(anatomy.has_value());
  QCOMPARE(anatomy->x, qvp::AnatomicalDirection::Left);
  QCOMPARE(anatomy->y, qvp::AnatomicalDirection::Posterior);
  QCOMPARE(anatomy->z, qvp::AnatomicalDirection::Superior);
}

void TestAnatomicalOrientation::parsesRai()
{
  const auto anatomy = qvp::parseAnatomicalOrientationAcronym("RAI");

  QVERIFY(anatomy.has_value());
  QCOMPARE(anatomy->x, qvp::AnatomicalDirection::Right);
  QCOMPARE(anatomy->y, qvp::AnatomicalDirection::Anterior);
  QCOMPARE(anatomy->z, qvp::AnatomicalDirection::Inferior);
}

void TestAnatomicalOrientation::parsesRip()
{
  const auto anatomy = qvp::parseAnatomicalOrientationAcronym("RIP");

  QVERIFY(anatomy.has_value());
  QCOMPARE(anatomy->x, qvp::AnatomicalDirection::Right);
  QCOMPARE(anatomy->y, qvp::AnatomicalDirection::Inferior);
  QCOMPARE(anatomy->z, qvp::AnatomicalDirection::Posterior);
}

void TestAnatomicalOrientation::rejectsUnknownPlaceholder()
{
  QVERIFY(!qvp::parseAnatomicalOrientationAcronym("???").has_value());
}

void TestAnatomicalOrientation::rejectsEmptyString()
{
  QVERIFY(!qvp::parseAnatomicalOrientationAcronym("").has_value());
}

void TestAnatomicalOrientation::rejectsPartialUnknown()
{
  QVERIFY(!qvp::parseAnatomicalOrientationAcronym("RA?").has_value());
}

void TestAnatomicalOrientation::rejectsInvalidLetters()
{
  QVERIFY(!qvp::parseAnatomicalOrientationAcronym("XYZ").has_value());
}

void TestAnatomicalOrientation::rejectsDuplicateAnatomicalFamily()
{
  QVERIFY(!qvp::parseAnatomicalOrientationAcronym("LLS").has_value());
}

void TestAnatomicalOrientation::rejectsLowercaseAcronym()
{
  QVERIFY(!qvp::parseAnatomicalOrientationAcronym("lps").has_value());
}

QTEST_MAIN(TestAnatomicalOrientation)

#include "TestAnatomicalOrientation.moc"
