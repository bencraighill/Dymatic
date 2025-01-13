#pragma once

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::uvec2>
	{
		static Node encode(const glm::uvec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::uvec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<uint32_t>();
			rhs.y = node[1].as<uint32_t>();
			return true;
		}
	};

	template<>
	struct convert<glm::quat>
	{
		static Node encode(const glm::quat& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::quat& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::mat4>
	{
		static Node encode(const glm::mat4& rhs)
		{
			Node node;

			for (uint32_t i = 0; i < 4; i++)
				node.push_back(convert<glm::vec4>::encode(rhs[i]));

			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::mat4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			for (uint32_t i = 0; i < 4; i++)
			{
				if (!node[i].IsSequence() || node[i].size() != 4)
					return false;

				rhs[i] = node[i].as<glm::vec4>();
			}

			return true;
		}
	};

#ifdef DY_USING_TRANSFORM
	template<>
	struct convert<Dymatic::Transform>
	{
		static Node encode(const Dymatic::Transform& rhs)
		{
			Node node;
			node["Translation"] = convert<glm::vec3>::encode(rhs.Translation);
			node["Rotation"] = convert<glm::quat>::encode(rhs.Rotation);
			node["Scale"] = convert<glm::vec3>::encode(rhs.Scale);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Dymatic::Transform& rhs)
		{
			if (!node.IsMap() || !node["Translation"] || !node["Rotation"] || !node["Scale"])
				return false;

			rhs.Translation = node["Translation"].as<glm::vec3>();
			rhs.Rotation = node["Rotation"].as<glm::quat>();
			rhs.Scale = node["Scale"].as<glm::vec3>();

			return true;
		}
	};
#endif

#ifdef DY_USING_FRACTION
	template<>
	struct YAML::convert<Dymatic::Fraction>
	{
		static Node encode(const Dymatic::Fraction& fraction)
		{
			Node node;
			node["Numerator"] = fraction.Numerator;
			node["Denominator"] = fraction.Denominator;
			return node;
		}

		static bool decode(const Node& node, Dymatic::Fraction& fraction)
		{
			if (!node.IsMap() || !node["Numerator"] || !node["Denominator"])
				return false;

			fraction.Numerator = node["Numerator"].as<int>();
			fraction.Denominator = node["Denominator"].as<int>();
			return true;
		}
	};
#endif

	template<>
	struct convert<Dymatic::UUID>
	{
		static Node encode(const Dymatic::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, Dymatic::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};

}

namespace Dymatic {

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::uvec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& q)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << q.x << q.y << q.z << q.w << YAML::EndSeq;
		return out;
	}

	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::mat4& mat)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq;

		for (uint32_t i = 0; i < 4; i++)
			out << YAML::BeginSeq << mat[i].x << mat[i].y << mat[i].z << mat[i].w << YAML::EndSeq;

		out << YAML::EndSeq;
		return out;
	}

#ifdef DY_USING_TRANSFORM
	static YAML::Emitter& operator<<(YAML::Emitter& out, const Transform& transform)
	{
		out << YAML::Flow;
		out << YAML::BeginMap;

		out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
		out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
		out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

		out << YAML::EndMap;
		return out;
	}
#endif

#ifdef DY_USING_FRACTION
	static YAML::Emitter& operator<<(YAML::Emitter& out, const Fraction& fraction)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Numerator" << YAML::Value << fraction.Numerator;
		out << YAML::Key << "Denominator" << YAML::Value << fraction.Denominator;
		out << YAML::EndMap;
		return out;
	}
#endif

}