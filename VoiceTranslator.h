#ifndef VOICETRANSLATOR_H
#define VOICETRANSLATOR_H

#include <QAudioRecorder>
#include <QStandardPaths>
#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

class VoiceTranslator : public QObject
{
    Q_OBJECT

    QNetworkAccessManager qam;
    QNetworkRequest request;
    QFile file;

    QString baseApi = "https://speech.googleapis.com/v1/speech:recognize";
    QString apiKey = "";

    QUrl url;
    QString filePath;

    const QDir location = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString fileName = QUuid::createUuid().toString(); // random unique recording name
    const int maxDuration = 10000; // maximum recording duration allowed
    const int minDuration = 1000; // minimium recording duration allowed

    QAudioRecorder audioRecorder;
    QAudioEncoderSettings audioSettings;

    QString command = ""; // last command
    QString error = ""; // last error
    int recordDuration = 2500; // recording duration in miliseconds
    bool running = false; // translation state

    int getRecordDuration() const {
        return this->recordDuration;
    }

    void setRecordDuration(int duration) {
        if (duration > maxDuration) {
            qInfo() << "duration is too big, setting to max value (miliseconds):" << maxDuration;
            duration = maxDuration;
        }

        if (duration < minDuration) {
            qInfo() << "duration is too small, setting to max value (miliseconds):" << minDuration;
            duration = minDuration;
        }

        if (duration != this->recordDuration) {
            this->recordDuration = duration;

            emit recordDurationChanged(recordDuration);
        }
    }

    bool getRunning() const {
        return this->running;
    }

    void setRunning(bool running) {
        if (running != this->running) {
            this->running = running;
            emit runningChanged(running);
        }
    }

    QString getCommand() const {
        return this->command;
    }

    void setCommand(QString command) {
        if (this->command != command) {
            this->command = command;
            emit commandChanged(command);
        }
    }

    QString getError() const {
        return this->error;
    }

    void setError(QString error) {
        if (this->error != error) {
            this->error = error;
            emit errorChanged(error);
        }
    }

    void translate() {

        if (!file.open(QIODevice::ReadOnly)) {
           qDebug()  << "cannot open file:" << file.errorString() << file.fileName();
           setRunning(false);
           setError(file.errorString());
           return;
        }

        QByteArray fileData = file.readAll();
        file.close();
        file.remove();

        QJsonDocument data {
            QJsonObject { {
                    "audio",
                    QJsonObject { {"content", QJsonValue::fromVariant(fileData.toBase64())} }
                },  {
                    "config",
                    QJsonObject {
                        {"encoding", "FLAC"},
                        {"languageCode", "en-US"},
                        {"model", "command_and_search"},
                        {"sampleRateHertz", audioSettings.sampleRate()}
                    }}
            }
        };

        qam.post(this->request, data.toJson(QJsonDocument::Compact));
    }

public:

    Q_PROPERTY(int recordDuration READ getRecordDuration WRITE setRecordDuration NOTIFY recordDurationChanged)
    Q_PROPERTY(QString command READ getCommand NOTIFY commandChanged)
    Q_PROPERTY(QString error READ getError NOTIFY errorChanged)
    Q_PROPERTY(bool running READ getRunning NOTIFY runningChanged)

    VoiceTranslator()
    {
        if (!this->location.exists())
            this->location.mkpath(".");

        this->filePath = location.filePath(fileName);

        this->audioSettings.setCodec("audio/x-flac");
        this->audioSettings.setSampleRate(16000);
        this->audioSettings.setQuality(QMultimedia::VeryHighQuality);
        this->audioRecorder.setEncodingSettings(audioSettings);
        this->audioRecorder.setOutputLocation(filePath);

        this->url.setUrl(this->baseApi);
        this->url.setQuery("key=" + this->apiKey);

        this->request.setUrl(this->url);
        this->request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        this->request.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);

        file.setFileName(this->filePath);

        connect(&audioRecorder, &QAudioRecorder::durationChanged, [this](auto duration)
        {
            qDebug() << "duration:" << duration;

            if (duration >= this->recordDuration && this->audioRecorder.state() != QAudioRecorder::StoppedState && this->audioRecorder.status() == QAudioRecorder::Status::RecordingStatus)
                this->audioRecorder.stop();
        });

        connect(&audioRecorder, &QAudioRecorder::statusChanged, [](auto status)
        {
            qDebug() << "status:" << status;
        });

        connect(&audioRecorder, &QAudioRecorder::stateChanged, [this](auto state)
        {
            qDebug() << "state:" << state;

            if (state == QAudioRecorder::StoppedState)
                this->translate();
        });

        connect(&audioRecorder, QOverload<QAudioRecorder::Error>::of(&QAudioRecorder::error), [this]
        {
            qDebug() << "error:" << audioRecorder.errorString();

            setRunning(false);
            setError(audioRecorder.errorString());
        });

        connect(&qam, &QNetworkAccessManager::finished, [this](QNetworkReply *response)
        {
            auto data = QJsonDocument::fromJson(response->readAll());
            response->deleteLater();

            qDebug() << data;

            auto error = data["error"]["message"];

            if (error.isUndefined()) {
                auto command = data["results"][0]["alternatives"][0]["transcript"].toString();

                setRunning(false);
                setCommand(command);

            } else {
                setRunning(false);
                setError(error.toString());
            }
        });
    }

    Q_INVOKABLE void start()
    {
        setError("");
        setCommand("");
        setRunning(true);
        audioRecorder.record();
    }

signals:
    void recordDurationChanged(qint64 duration);
    void runningChanged(bool running);
    void commandChanged(QString text);
    void errorChanged(QString text);
};

#endif // VOICETRANSLATOR_H
