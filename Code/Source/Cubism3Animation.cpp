#include "StdAfx.h"

#include <AZCore/JSON/rapidjson.h>
#include <AZCore/JSON/document.h>

#include "Cubism3Animation.h"

#include "Cubism3EditorData.h"

#include "Cubism3UIComponent.h"

namespace Cubism3 {
	Cubism3Animation::Cubism3Animation() {
		this->m_loaded = false;
		this->m_paramGroup = nullptr;
		this->m_partsGroup = nullptr;
		this->m_floatBlendFunc = FloatBlend::Default;
		this->m_time = 0.0f;
		this->m_weight = 1.0f;
		this->m_playedOnce = false;
		this->m_playing = false;
		this->m_drawables = nullptr;
	}

	Cubism3Animation::~Cubism3Animation() {
		if (this->m_curves.size() > 0) {
			for (Curve *c : this->m_curves) { //for each curve.
				for (int i = 0; i < c->m_segments.size(); i++) delete c->m_segments[i].second; //delete the calculation for each segment
				c->m_segments.clear(); //clear the segments
			}
			this->m_curves.clear();
		}
	}

	void Cubism3Animation::Load(MotionAssetRef asset) {
		this->m_loaded = false;

		if (this->m_curves.size() > 0) {
			for (Curve *c : this->m_curves) { //for each curve.
				for (int i = 0; i < c->m_segments.size(); i++) delete c->m_segments[i].second; //delete the calculation for each segment
				c->m_segments.clear(); //clear the segments
			}
			this->m_curves.clear();
		}

		if (asset.GetAssetPath().empty()) return;
		CLOG("[Cubism3] Reading %s.", asset.GetAssetPath().c_str());

		bool error = false;

		//load the json file
		AZ::IO::HandleType fileHandler;
		gEnv->pFileIO->Open(asset.GetAssetPath().c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandler);

		AZ::u64 size;
		gEnv->pFileIO->Size(fileHandler, size);

		char *jsonBuff = (char *)malloc(size + 1);
		gEnv->pFileIO->Read(fileHandler, jsonBuff, size);

		gEnv->pFileIO->Close(fileHandler);

		jsonBuff[size] = '\0';

		//parse the json file
		rapidjson::Document d;
		d.Parse(jsonBuff);
		free(jsonBuff);

		//parse check
		if (!d.IsObject()) {
			CRY_ASSERT_MESSAGE(false, "JSON file not valid.");
			error = true;
		}

		//file version check
		if (!error) {
			if (d.HasMember("Version")) {
				if (d["Version"].GetInt() != 3) {
					CRY_ASSERT_MESSAGE(false, "Animation version incorrect.");
					error = true;
				}
			} else {
				CRY_ASSERT_MESSAGE(false, "Animation file does not have a version section.");
				error = true;
			}
		}

		//get meta data
		if (!error) {
			if (d.HasMember("Meta")) {
				if (d["Meta"].HasMember("Duration")) this->m_meta.m_duration = (float)d["Meta"]["Duration"].GetDouble();
				else {
					CRY_ASSERT_MESSAGE(false, "Animation file does not have Duration in Meta section.");
					error = true;
				}

				if (d["Meta"].HasMember("Fps")) this->m_meta.m_fps = (float)d["Meta"]["Fps"].GetDouble();
				else {
					CRY_ASSERT_MESSAGE(false, "Animation file does not have Fps in Meta section.");
					error = true;
				}

				if (d["Meta"].HasMember("Loop")) this->m_meta.m_loop = d["Meta"]["Loop"].GetBool();
				else {
					CRY_ASSERT_MESSAGE(false, "Animation file does not have Loop in Meta section.");
					error = true;
				}

				if (d["Meta"].HasMember("CurveCount")) this->m_meta.m_curveCount = (float)d["Meta"]["CurveCount"].GetInt();
				else {
					CRY_ASSERT_MESSAGE(false, "Animation file does not have CurveCount in Meta section.");
					error = true;
				}

				if (d["Meta"].HasMember("TotalSegmentCount")) this->m_meta.m_totalSegCount = (float)d["Meta"]["TotalSegmentCount"].GetInt();
				else {
					CRY_ASSERT_MESSAGE(false, "Animation file does not have TotalSegmentCount in Meta section.");
					error = true;
				}

				if (d["Meta"].HasMember("TotalPointCount")) this->m_meta.m_totalPointCount = (float)d["Meta"]["TotalPointCount"].GetInt();
				else {
					CRY_ASSERT_MESSAGE(false, "Animation file does not have TotalPointCount in Meta section.");
					error = true;
				}
			} else {
				CRY_ASSERT_MESSAGE(false, "Animation file does not have a Meta section.");
				error = true;
			}
		}

		//get curves
		if (!error) {
			if (d.HasMember("Curves")) {
				for (unsigned int i = 0; i < d["Curves"].Size(); i++) {
					CLOG("[Cubism3] Curve: %i", i);
					Curve * c = new Curve();
					c->m_id = -1;

					//get the target
					if (d["Curves"][i].HasMember("Target")) {
						AZStd::string targetStr = d["Curves"][i]["Target"].GetString();

						if (targetStr.compare("Model") == 0) c->m_target = Model;
						if (targetStr.compare("Parameter") == 0) c->m_target = Parameter;
						if (targetStr.compare("Part") == 0) c->m_target = Part;
						CLOG("[Cubism3] - Target: %s", targetStr);
					} else {
						CLOG("[Cubism3] - Skipping %i. Target not found.", i);
						delete c;
						continue; //ignore this curve
					}

					//get the id
					if (d["Curves"][i].HasMember("Id")) {
						switch (c->m_target) {
						case Model:
							c->m_idStr = "Opacity";
							break;
						case Parameter:
						case Part:
							c->m_idStr = d["Curves"][i]["Id"].GetString();
							break;
						}
						CLOG("[Cubism3] - ID: %s", c->m_idStr.c_str());
					} else {
						CLOG("[Cubism3] - Skipping %i. Id not found.", i);
						delete c;
						continue; //ignore this curve
					}

					//get the segments
					if (d["Curves"][i].HasMember("Segments")) {
						AZStd::vector<AZStd::pair<float, float>> segments; //time -> data
						AZStd::vector<int> segmentTypes;

						{ //get all data;
							CLOG("[Cubism3] - Reading Segment Data.");
							AZStd::pair<float, float> segment; //time -> data
							bool timeArea = true;

							for (unsigned int s = 0, t = 2; s < d["Curves"][i]["Segments"].Size(); s++) {
								if (t != s) {
									if (timeArea) {
										segment.first = (float)d["Curves"][i]["Segments"][s].GetDouble();
										CLOG("[Cubism3] -- Time: %f", segment.first);
									} else {
										segment.second = (float)d["Curves"][i]["Segments"][s].GetDouble();
										segments.push_back(segment);
										CLOG("[Cubism3] -- Data: %f", segment.second);
										segment.first = 0.0f;
										segment.second = 0.0f;
									}
									timeArea = !timeArea;
								} else {
									int segtype = d["Curves"][i]["Segments"][s].GetInt();
									segmentTypes.push_back(segtype);

									segtype == BEZIER ? t += 7 : t += 3;
									timeArea = true;
									CLOG("[Cubism3] -- Type: %i", segtype);
								}
							}
						}

						{ //parse all data
							CLOG("[Cubism3] - Parsing Data.");
							for (unsigned int s = 0, si = 0; s < segmentTypes.size(); s++) {
								AZStd::pair<float, float> segment = segments.at(si); //keyframe -> data
								Calc * calc = nullptr;

								switch (segmentTypes.at(s)) {
								case LINEAR: //2 data points
								{
									Linear * l = new Linear();
									l->m_data[0] = segment;
									l->m_data[1] = segments.at(si + 1);
									calc = l;
									si += 1;

									CLOG("[Cubism3] -- Linear Type:");
									CLOG("[Cubism3] --- 0: %f, %f", l->m_data[0].first, l->m_data[0].second);
									CLOG("[Cubism3] --- 1: %f, %f", l->m_data[1].first, l->m_data[1].second);
									break;
								}
								case BEZIER: //4 data points
								{
									Bezier * b = new Bezier();
									b->m_data[0] = segment;
									b->m_data[1] = segments.at(si + 1);
									b->m_data[2] = segments.at(si + 2);
									b->m_data[3] = segments.at(si + 3);
									calc = b;
									si += 3;

									CLOG("[Cubism3] -- Bezier Type:");
									CLOG("[Cubism3] --- 0: %f, %f", b->m_data[0].first, b->m_data[0].second);
									CLOG("[Cubism3] --- 1: %f, %f", b->m_data[1].first, b->m_data[1].second);
									CLOG("[Cubism3] --- 2: %f, %f", b->m_data[2].first, b->m_data[2].second);
									CLOG("[Cubism3] --- 3: %f, %f", b->m_data[3].first, b->m_data[3].second);
									break;
								}
								case STEPPED: //1 data point
								{
									Stepped * st = new Stepped();
									st->m_data = segments.at(si + 1);
									st->m_type = STEPPED;
									si += 1;
									calc = st;

									CLOG("[Cubism3] -- Stepped Type:");
									CLOG("[Cubism3] --- %f, %f", st->m_data.first, st->m_data.second);
									break;
								}
								case ISTEPPED: //1 data point
								{
									Stepped * ist = new Stepped();
									ist->m_data = segment;
									ist->m_type = ISTEPPED;
									si += 1;
									calc = ist;

									CLOG("[Cubism3] -- Inverse Stepped Type:");
									CLOG("[Cubism3] --- %f, %f", ist->m_data.first, ist->m_data.second);
									break;
								}
								}

								c->m_segments.push_back({ segment.first, calc });
							}
						}
					} else {
						CLOG("[Cubism3] - Skipping %i. Segments not found.", i);
						delete c;
						continue; //ignore this curve
					}

					this->m_curves.push_back(c);
				}
			} else {
				CRY_ASSERT_MESSAGE(false, "Animation file does not have a Curves section.");
				error = true;
			}
		}

		if (!error) this->m_loaded = true;
		else {
			if (this->m_curves.size() > 0) {
				for (Curve *c : this->m_curves) { //for each curve.
					for (int i = 0; i < c->m_segments.size(); i++) delete c->m_segments[i].second; //delete the calculation for each segment
					c->m_segments.clear(); //clear the segments
				}
				this->m_curves.clear();
			}
		}

		#ifdef ENABLE_CUBISM3_DEBUG
		CLOG("[Cubism3] Animation Loaded.");
		CLOG("[Cubism3] Number of Curves: %i", this->m_curves.size());
		for (Curve * c : this->m_curves) {
			CLOG("[Cubism3] Curve: %s", c->m_idStr.c_str());
			CLOG("[Cubism3] - Target: %i", c->m_target);
			CLOG("[Cubism3] - Segments: %i", c->m_segments.size());

			for (AZStd::pair<float, Calc *> s : c->m_segments) {
				AZStd::string type = "Unknown";

				switch (s.second->getType()) {
				case LINEAR:
					type = "Linear";
					break;
				case BEZIER:
					type = "Bezier";
					break;
				case STEPPED:
					type = "Stepped";
					break;
				case ISTEPPED:
					type = "Inverse Stepped";
					break;
				}

				CLOG("[Cubism3] -- %f -> %s", s.first, type.c_str());
			}
		}
		#endif

		this->SetParametersAndParts(this->m_paramGroup, this->m_partsGroup);
	}

	void Cubism3Animation::SetParametersAndParts(ModelParametersGroup * paramGroup, ModelPartsGroup * partsGroup){
		if (paramGroup == nullptr || partsGroup == nullptr) {
			this->m_paramGroup = nullptr;
			this->m_partsGroup = nullptr;

			//remove id association.
			for (Curve *c : this->m_curves) c->m_id = -1;
		} else {
			this->m_paramGroup = paramGroup;
			this->m_partsGroup = partsGroup;

			//create id associaton.
			for (Curve *c : this->m_curves) {
				switch (c->m_target) {
				case Model:
					//ignore
					break;
				case Parameter:
					c->m_id = this->m_paramGroup->find(c->m_idStr);
					break;
				case Part:
					c->m_id = this->m_partsGroup->find(c->m_idStr);
					break;
				}
			}

			//*this->m_paramGroup->at(0)->val = 0.0f;
			//*this->m_partsGroup->at(0)->val = 0.0f;
		}
	}

	void Cubism3Animation::SetLooping(bool looping) {
		this->m_meta.m_loop = looping;
		this->m_playedOnce = false;
	}

	bool Cubism3Animation::IsPlaying() {
		return this->m_playing;
	}

	bool Cubism3Animation::IsStopped() {
		return this->m_playing && (this->m_time == 0.0f || this->m_time == this->m_meta.m_duration);
	}

	bool Cubism3Animation::IsPaused() {
		return this->m_playing && (this->m_time != 0.0f || this->m_time != this->m_meta.m_duration);
	}

	void Cubism3Animation::Play() {
		this->m_playing = true;
		if (this->m_playedOnce) this->m_playedOnce = false;
	}

	void Cubism3Animation::Stop() {
		this->m_playing = false;
		this->m_playedOnce = false;
		this->m_time = 0.0f;
		this->UpdateCurves();
	}

	void Cubism3Animation::Pause() {
		this->m_playing = false;
	}

	void Cubism3Animation::Tick(float delta) {
		if (this->m_paramGroup == nullptr && this->m_partsGroup == nullptr) return; //if we dont have any parameters or parts to work with dont deal with the animation.

		if (!this->m_playedOnce && this->m_playing) { //if we havent played once and we are playing
			this->m_time += delta;

			if (this->m_time > this->m_meta.m_duration) { //if we have exceded the animation time
				if (!this->m_meta.m_loop) { //if we are not looping
					this->m_playedOnce = true; //make sure that we dont loop
					this->m_playing = false; //make sure we are stopped
					this->m_time = this->m_meta.m_duration; //stop at the end of the animation
				} else while (this->m_time > this->m_meta.m_duration) { this->m_time -= this->m_meta.m_duration; } //trim the time
			}

			this->UpdateCurves();
		}
	}

	void Cubism3Animation::Reset() {
		this->m_time = 0.0f;
		this->m_playedOnce = false;
		this->UpdateCurves();
	}

	void Cubism3Animation::UpdateCurves() {
		for (Curve * c : this->m_curves) { //for each curve
			float val = 0.0f;
			for (int i = c->m_segments.size() - 1; i >= 0; i--) { //for each segment
				if (this->m_time >= c->m_segments[i].first) { //find the keyframe that we are in
					val = c->m_segments[i].second->calculate(this->m_time); //calculate the animation value
					break;
				}
			}

			if (c->m_id != -1) {
				ModelAnimation * target = nullptr;
				switch (c->m_target) {
				case Parameter:
					target = this->m_paramGroup->at(c->m_id);
					break;
				case Part:
					target = this->m_partsGroup->at(c->m_id);
					break;
				}

				if (target != nullptr) {
					target->m_animMutex.Lock();
					target->m_animVal = this->m_floatBlendFunc(target->m_animVal, val, this->m_weight); //blend the animation
					target->m_animDirty = true;
					target->m_animMutex.Unlock();
				}
			} else if (c->m_target == Model) {
				if (this->m_drawables)
					for (Cubism3Drawable* d : *this->m_drawables)
						d->m_animOpacity = this->m_floatBlendFunc(d->m_animOpacity, val/100.0f, this->m_weight);
			}
		}
	}

	float Cubism3Animation::Linear::calculate(float time) {
		float t = (time - this->m_data[0].first) / (this->m_data[1].first - this->m_data[0].first);
		return this->m_data[0].second + ((this->m_data[1].second - this->m_data[0].second) * t);
	}

	static inline AZStd::pair<float, float> Lerp(AZStd::pair<float, float> a, AZStd::pair<float, float> b, const float t) {
		return {
			a.first + ((b.first - a.first) * t),
			a.second + ((b.second - a.second) * t)
		};
	}

	float Cubism3Animation::Bezier::calculate(float time) {
		float t = (time - this->m_data[0].first) / (this->m_data[3].first - this->m_data[0].first);

		AZStd::pair<float, float> p01, p12, p23, p012, p123;

		p01 = Lerp(this->m_data[0], this->m_data[1], t);
		p12 = Lerp(this->m_data[1], this->m_data[2], t);
		p23 = Lerp(this->m_data[2], this->m_data[3], t);

		p012 = Lerp(p01, p12, t);
		p123 = Lerp(p12, p23, t);

		return Lerp(p012,p123,t).second;
	}

	void Cubism3Animation::Reflect(AZ::SerializeContext* serializeContext) {
		serializeContext->Class<Cubism3Animation>()
			->SerializerForEmptyClass();
	}

	namespace FloatBlend {
		float Default(float base, float value, float weight) {
			return value * weight;
		}

		float Additive(float base, float value, float weight) {
			return base + (value * weight);
		}
	}
}