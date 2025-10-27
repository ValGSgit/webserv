#include "../../includes/utils/SessionManager.hpp"
#include <sstream>
#include <cstdlib>

/**
 * Constructor - Initializes SessionManager with 1 hour default timeout
 */
SessionManager::SessionManager() : _session_timeout(3600) {
	srand(time(NULL));
}

/**
 * Destructor - Cleans up all sessions
 */
SessionManager::~SessionManager() {
	_sessions.clear();
}

/**
 * Generates a unique session ID using timestamp and random numbers
 *
 * @return Unique session identifier string
 */
std::string SessionManager::generateSessionId() {
	std::ostringstream oss;
	oss << time(NULL) << "_" << rand() << "_" << rand();
	return oss.str();
}

/**
 * Creates a new session and stores it in the session map
 *
 * @return The newly created session ID
 */
std::string SessionManager::createSession() {
	std::string session_id = generateSessionId();

	SessionData& session = _sessions[session_id];
	session.session_id = session_id;
	session.created_at = time(NULL);
	session.last_accessed = time(NULL);
	session.expires_at = time(NULL) + _session_timeout;

	return session_id;
}

/**
 * Retrieves an existing session by ID, updates last access time
 * Also checks if session has expired
 *
 * @param session_id The session identifier to look up
 * @return Pointer to SessionData or NULL if not found or expired
 */
SessionData* SessionManager::getSession(const std::string& session_id) {
	std::map<std::string, SessionData>::iterator it = _sessions.find(session_id);
	if (it == _sessions.end()) {
		return NULL;
	}

	SessionData& session = it->second;

	// Check if expired
	time_t now = time(NULL);
	if (now - session.last_accessed > _session_timeout) {
		_sessions.erase(it);
		return NULL;
	}

	// Update last accessed time
	session.last_accessed = now;
	return &session;
}

/**
 * Destroys a session by removing it from the session map
 *
 * @param session_id The session identifier to destroy
 */
void SessionManager::destroySession(const std::string& session_id) {
	std::map<std::string, SessionData>::iterator it = _sessions.find(session_id);
	if (it != _sessions.end()) {
		// Remove username mapping if exists
		std::string username = it->second.data["username"];
		if (!username.empty()) {
			_username_to_session.erase(username);
		}
		_sessions.erase(it);
	}
}

/**
 * Iterates through all sessions and removes expired ones
 * Should be called periodically by the server
 */
void SessionManager::cleanExpiredSessions() {
	time_t now = time(NULL);
	std::map<std::string, SessionData>::iterator it = _sessions.begin();

	while (it != _sessions.end()) {
		if (now - it->second.last_accessed > _session_timeout) {
			_sessions.erase(it++);
		} else {
			++it;
		}
	}
}

/**
 * Sets the session timeout duration
 *
 * @param seconds Timeout in seconds (default is 3600 = 1 hour)
 */
void SessionManager::setSessionTimeout(int seconds) {
	_session_timeout = seconds;
}

/**
 * Gets the number of currently active sessions
 *
 * @return Number of active sessions
 */
size_t SessionManager::getActiveSessionCount() const {
	return _sessions.size();
}

/**
 * Check if a username already has an active session
 *
 * @param username The username to check
 * @return Session ID if active session exists, empty string otherwise
 */
std::string SessionManager::getSessionByUsername(const std::string& username) {
	// Check if username has a mapped session
	std::map<std::string, std::string>::iterator it = _username_to_session.find(username);
	if (it != _username_to_session.end()) {
		std::string session_id = it->second;
		
		// Verify the session is still valid
		SessionData* session = getSession(session_id);
		if (session && session->expires_at > time(NULL)) {
			return session_id; // Active session found
		} else {
			// Session expired, clean up mapping
			_username_to_session.erase(it);
		}
	}
	return ""; // No active session found
}

/**
 * Register a username with a session ID
 *
 * @param session_id The session identifier
 * @param username The username to register
 */
void SessionManager::registerUsername(const std::string& session_id, const std::string& username) {
	_username_to_session[username] = session_id;
}
