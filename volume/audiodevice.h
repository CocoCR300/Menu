#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <QObject>

class AudioEngine;

typedef enum AudioDeviceType {
    Sink = 0,
    Source = 1,
    PulseAudioDeviceTypeLength
} AudioDeviceType;

class AudioDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(AudioDeviceType type READ type CONSTANT)

public:
    AudioDevice(AudioDeviceType t, AudioEngine *engine, QObject *parent = nullptr);
    ~AudioDevice();

    // the volume can range from 0 to 100.
    int volume() const { return m_volume; }
    bool mute() const { return m_mute; }
    AudioDeviceType type() const { return m_type; }
    const QString &name() const { return m_name; }
    const QString &description() const { return m_description; }
    uint index() const { return m_index; }

    void setName(const QString &name);
    void setDescription(const QString &description);
    void setIndex(uint index);

    AudioEngine* engine() { return m_engine; }

public slots:
    // the volume can range from 0 to 100.
    void setVolume(int volume);
    void setVolumeNoCommit(int volume);
    void toggleMute();
    void setMute(bool state);
    void setMuteNoCommit(bool state);

signals:
    void volumeChanged(int volume);
    void muteChanged(bool state);
    void nameChanged(const QString &name);
    void descriptionChanged(const QString &description);
    void indexChanged(uint index);

private:
    AudioEngine *m_engine;
    int m_volume; // the volume can range from 0 to 100.
    bool m_mute;
    AudioDeviceType m_type;
    QString m_name;
    uint m_index;
    QString m_description;
};

#endif // AUDIODEVICE_H
