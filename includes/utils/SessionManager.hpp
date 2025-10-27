#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "../webserv.hpp"

struct SessionData {
	std::string session_id;
	time_t created_at;
	time_t last_accessed;
	std::map<std::string, std::string> data;

	SessionData() : created_at(0), last_accessed(0) {}
};

class SessionManager {
	private:
		std::map<std::string, SessionData> _sessions;
		int _session_timeout;

		std::string generateSessionId();

	public:
		SessionManager();
		~SessionManager();

		std::string createSession();
		SessionData* getSession(const std::string& session_id);
		void destroySession(const std::string& session_id);
		void cleanExpiredSessions();
		void setSessionTimeout(int seconds);
		size_t getActiveSessionCount() const;
};

#endif
