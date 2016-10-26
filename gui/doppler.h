#include <QtGui>
#include <QtCore>

class Doppler : public QWidget {
  Q_OBJECT
public:
  Doppler( QWidget *parent=0);
  QSizePolicy sizePolicy() const;
public slots:
  void setData(double phase, double power);
signals:
protected:
  void paintEvent(QPaintEvent *);
private:
  double phase; // degrees
  double power; // dBm
};
