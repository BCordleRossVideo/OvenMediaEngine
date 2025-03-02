//==============================================================================
//
//  Transcode
//
//  Created by Kwon Keuk Han
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================
#include "stream.h"

#include <random>

#include "application.h"

#define OV_LOG_TAG "Stream"

using namespace cmn;

namespace info
{
	Stream::Stream(const info::Application &app_info, StreamSourceType source)
	{
		_app_info = std::make_shared<info::Application>(app_info);

		// ID RANDOM 생성
		SetId(ov::Random::GenerateUInt32() - 1);

		_created_time = std::chrono::system_clock::now();
		_source_type = source;
	}

	Stream::Stream(const info::Application &app_info, info::stream_id_t stream_id, StreamSourceType source)
	{
		_app_info = std::make_shared<info::Application>(app_info);

		_id = stream_id;
		_created_time = std::chrono::system_clock::now();
		_source_type = source;
	}

	Stream::Stream(const Stream &stream)
	{
		_id = stream._id;
		_name = stream._name;
		_source_type = stream._source_type;
		_source_url = stream._source_url;
		_created_time = stream._created_time;
		_app_info = stream._app_info;
		_origin_stream = stream._origin_stream;
		_has_video_track = stream._has_video_track;
		_has_audio_track = stream._has_audio_track;

		_tracks = stream._tracks;
		_playlists = stream._playlists;
		_representation_type = stream._representation_type;
	}

	Stream::Stream(StreamSourceType source)
	{
		_source_type = source;
		_created_time = std::chrono::system_clock::now();
	}

	Stream::~Stream()
	{
		logd("DEBUG", "Stream (%s / %s) Destroyed", GetName().CStr(), GetUUID().CStr());
	}

	bool Stream::operator==(const Stream &stream_info) const
	{
		if (_id == stream_info._id && *_app_info == *stream_info._app_info)
		{
			return true;
		}

		return false;
	}

	void Stream::SetId(info::stream_id_t id)
	{
		_id = id;
	}

	info::stream_id_t Stream::GetId() const
	{
		return _id;
	}

	ov::String Stream::GetUri()
	{
		// #vhost name#appname/stream name
		ov::String vhost_app_name = _app_info != nullptr ? _app_info->GetName().CStr() : "Unknown";
		return ov::String::FormatString("%s/%s", vhost_app_name.CStr(), GetName().CStr());
	}

	void Stream::SetMsid(uint32_t msid)
	{
		_msid = msid;
	}
	
	uint32_t Stream::GetMsid()
	{
		return _msid;
	}

	ov::String Stream::GetUUID() const
	{
		if (_app_info == nullptr)
		{
			return "";
		}

		return ov::String::FormatString("%s/%s/%s", _app_info->GetUUID().CStr(), GetName().CStr(), IsInputStream() ? "i" : "o");
	}

	ov::String Stream::GetName() const
	{
		return _name;
	}

	void Stream::SetName(ov::String name)
	{
		_name = std::move(name);
	}

	ov::String Stream::GetMediaSource() const
	{
		return _source_url;
	}
	void Stream::SetMediaSource(ov::String url)
	{
		_source_url = url;
	}

	bool Stream::IsInputStream() const
	{
		return IsOutputStream() == false;
	}

	bool Stream::IsOutputStream() const
	{
		return GetSourceType() == StreamSourceType::Transcoder || GetLinkedInputStream() != nullptr;
	}

	void Stream::LinkInputStream(const std::shared_ptr<Stream> &stream)
	{
		_origin_stream = stream;
	}

	const std::shared_ptr<Stream> Stream::GetLinkedInputStream() const
	{
		return _origin_stream;
	}

	// Only used in OVT provider
	void Stream::SetOriginStreamUUID(const ov::String &uuid)
	{
		_origin_stream_uuid = uuid;
	}

	ov::String Stream::GetOriginStreamUUID() const
	{
		return _origin_stream_uuid;
	}

	const std::chrono::system_clock::time_point &Stream::GetInputStreamCreatedTime() const
	{
		if (GetLinkedInputStream() != nullptr)
		{
			return GetLinkedInputStream()->GetCreatedTime();
		}

		return GetCreatedTime();
	}

	const std::chrono::system_clock::time_point &Stream::GetCreatedTime() const
	{
		return _created_time;
	}

	uint32_t Stream::GetUptimeSec()
	{
		auto current = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(current - GetCreatedTime()).count();
	}

	StreamSourceType Stream::GetSourceType() const
	{
		return _source_type;
	}

	StreamRepresentationType Stream::GetRepresentationType() const {
		return _representation_type;		
	}
	
	void Stream::SetRepresentationType(const StreamRepresentationType &type) {
		_representation_type = type;
	}

	bool Stream::AddTrack(const std::shared_ptr<MediaTrack> &track)
	{
		// If there is an existing track with the same track id, it will be deleted.
		auto item = _tracks.find(track->GetId());
		if (item != _tracks.end())
		{
			_tracks.erase(item);
		}

		auto result = _tracks.insert(std::make_pair(track->GetId(), track)).second;

		if (track->GetMediaType() == cmn::MediaType::Video)
		{
			_has_video_track = true;
		}
		else if (track->GetMediaType() == cmn::MediaType::Audio)
		{
			_has_audio_track = true;
		}

		return result;
	}

	const std::shared_ptr<MediaTrack> Stream::GetTrack(int32_t id) const
	{
		auto item = _tracks.find(id);
		if (item == _tracks.end())
		{
			return nullptr;
		}

		return item->second;
	}

	// Get Track by name
	const std::shared_ptr<MediaTrack> Stream::GetTrack(const ov::String &name) const
	{
		for (auto &item : _tracks)
		{
			if (item.second->GetName() == name)
			{
				return item.second;
			}
		}

		return nullptr;
	}

	const std::shared_ptr<MediaTrack> Stream::GetFirstTrack(const cmn::MediaType &type) const
	{
		for (auto &item : _tracks)
		{
			if (item.second->GetMediaType() == type)
			{
				return item.second;
			}
		}

		return nullptr;
	}

	const std::map<int32_t, std::shared_ptr<MediaTrack>> &Stream::GetTracks() const
	{
		return _tracks;
	}

	bool Stream::AddPlaylist(const std::shared_ptr<Playlist> &playlist)
	{
		_playlists.emplace(playlist->GetFileName(), playlist);
		return true;
	}

	std::shared_ptr<const Playlist> Stream::GetPlaylist(const ov::String &file_name) const
	{
		auto item = _playlists.find(file_name);
		if (item == _playlists.end())
		{
			return nullptr;
		}

		return item->second;
	}

	const std::map<ov::String, std::shared_ptr<Playlist>> &Stream::GetPlaylists() const
	{
		return _playlists;
	}

	const char *Stream::GetApplicationName()
	{
		if (_app_info == nullptr)
		{
			return "Unknown";
		}

		return _app_info->GetName().CStr();
	}

	ov::String Stream::GetInfoString()
	{
		ov::String out_str = ov::String::FormatString("\n[Stream Info]\nid(%u), msid(%u), output(%s), SourceType(%s), RepresentationType(%s), Created Time (%s) UUID(%s)\n",
													  GetId(), GetMsid(), GetName().CStr(), ::StringFromStreamSourceType(_source_type).CStr(), ::StringFromStreamRepresentationType(_representation_type).CStr(),
													  ov::Converter::ToString(_created_time).CStr(), GetUUID().CStr());
		if (GetLinkedInputStream() != nullptr)
		{
			out_str.AppendFormat("\t>> Origin Stream Info\n\tid(%u), output(%s), SourceType(%s), Created Time (%s)\n",
								 GetLinkedInputStream()->GetId(), GetLinkedInputStream()->GetName().CStr(), ::StringFromStreamSourceType(GetLinkedInputStream()->GetSourceType()).CStr(),
								 ov::Converter::ToString(GetLinkedInputStream()->GetCreatedTime()).CStr());
		}

		if (GetOriginStreamUUID().IsEmpty() == false)
		{
			out_str.AppendFormat("\t>> Origin Stream UUID : %s\n", GetOriginStreamUUID().CStr());
		}

		for (auto it = _tracks.begin(); it != _tracks.end(); ++it)
		{
			auto track = it->second;

			out_str.AppendFormat("\n\t%s", track->GetInfoString().CStr());
		}

		return out_str;
	}

	void Stream::ShowInfo()
	{
		logi("Monitor", "%s", GetInfoString().CStr());
	}
}  // namespace info