// Polls `ss -tnoi` (via the helper) at 1 Hz for connections to/from a peer
// ip:port, or (match "local") for connections on a local port, e.g. a
// WebSocket server on this PC. Tracks state changes, retransmissions, RTO and
// backoff so the TCP stall after a link outage can be seen and exported.
#pragma once

#include "Parsers.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class QAbstractSeries;
class HelperClient;
class EventLog;

class SocketMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString peerIp READ peerIp WRITE setPeerIp NOTIFY configChanged)
    Q_PROPERTY(int peerPort READ peerPort WRITE setPeerPort NOTIFY configChanged)
    // "peer": the remote end is ip:port. "local": the local port is `peerPort`
    // (a server here, clients on random ports); peerIp is an optional client filter.
    Q_PROPERTY(QString match READ match WRITE setMatch NOTIFY configChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QVariantList sockets READ sockets NOTIFY updated)
    Q_PROPERTY(QString lastError READ lastError NOTIFY updated)
    Q_PROPERTY(QString tracked READ tracked NOTIFY updated)
public:
    SocketMonitor(HelperClient *helper, EventLog *log, QObject *parent = nullptr);

    QString peerIp() const { return m_ip; }
    int peerPort() const { return m_port; }
    QString match() const { return m_match; }
    bool running() const { return m_timer.isActive(); }
    QVariantList sockets() const { return m_rows; }
    QString lastError() const { return m_lastError; }
    QString tracked() const { return m_trackedKey; }
    void setPeerIp(const QString &ip);
    void setPeerPort(int port);
    void setMatch(const QString &match);

    // Takes ip and port from "ws://ip:port/…", "wss://ip" or "ip:port".
    // Returns an error message, empty on success.
    Q_INVOKABLE QString setFromUrl(const QString &url);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clear();
    // key: rto, retrans, backoff, sendq, rtt, timer. Returns {xmin, xmax, ymax}.
    Q_INVOKABLE QVariantMap updateSeries(QAbstractSeries *series, const QString &key, int windowSeconds) const;
    Q_INVOKABLE bool exportCsv(const QUrl &file);

Q_SIGNALS:
    void configChanged();
    void runningChanged();
    void updated();

private:
    struct Point {
        double t;
        dlsim::TcpSocketInfo s;
        bool present;
    };
    void poll();
    void process(const QList<dlsim::TcpSocketInfo> &socks);

    HelperClient *m_helper;
    EventLog *m_log;
    QString m_ip = QStringLiteral("192.0.2.10");
    int m_port = 4555;
    QString m_match = QStringLiteral("peer");
    QTimer m_timer;
    QElapsedTimer m_clock;
    bool m_inFlight = false;
    QVariantList m_rows;
    QString m_lastError;
    QHash<QString, QString> m_states; // key -> state
    QHash<QString, int> m_backoff;
    QString m_trackedKey;
    QList<Point> m_history;                              // tracked socket over time
    QList<QPair<double, dlsim::TcpSocketInfo>> m_allRows; // every socket, every poll, for CSV
};
