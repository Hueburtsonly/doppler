//#include <qapplication.h>
//#include <qpushbutton.h>

#include <QtGui>
#include <QtCore>
//#include <QButton>

#include "doppler.h"

#include <stdio.h>
#include <pthread.h>

Doppler* widget = 0;

void *event_thread_func(void *ctx) {
  while (1) {
    double phase;
    double power;
    if (2 != scanf("%lf,%lf", &phase, &power)) {
      continue;
    }
    widget->setData(phase, power);
  }
  return NULL;
}

int main(int argc, char **argv) {
  QApplication a(argc, argv);
  QMainWindow x;

  widget = new Doppler(0);
  //hello.resize( 500, 400 );

  x.setCentralWidget(widget);
  x.show();

  pthread_t event_thread;
  pthread_create(&event_thread, NULL, &event_thread_func, 0);

  return a.exec();
}

Doppler::Doppler(QWidget *parent) : QWidget(parent) {}

void Doppler::paintEvent(QPaintEvent*) {
    QString s1 = "Width = " + QString::number( width() );
    QString s2 = "Height = " + QString::number( height() );
    QString s3 = "Phase = " + QString::number( phase );
    QString s4 = "Power = " + QString::number( power );
    QPainter p( this );
    p.drawText( 20, 20, s1 );
    p.drawText( 20, 40, s2 );
    p.drawText( 20, 60, s3 );
    p.drawText( 20, 80, s4 );
}

QSizePolicy Doppler::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

void Doppler::setData(double phase, double power) {
  this->phase = phase;
  this->power = power;
  repaint();
}