#ifndef FRONTEND_DATETIMEPICKER_H
#define FRONTEND_DATETIMEPICKER_H

#include <QDateTime>
#include <QPushButton>

class DateTimeButton final : public QPushButton {
    Q_OBJECT

public:
    DateTimeButton(const QString &title, const QDateTime &dateTime, QWidget *parent);

    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dateTime);
    void setMinimumDateTime(const QDateTime &minimum);

signals:
    void dateTimeChanged();

private:
    QDateTime value;
    QDateTime minimumDateTime;
};

#endif // FRONTEND_DATETIMEPICKER_H
