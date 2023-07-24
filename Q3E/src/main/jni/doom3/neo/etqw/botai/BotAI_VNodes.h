// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTAI_NODES_H__
#define __BOTAI_NODES_H__

#define OLD_NODE_VERSION	7 // 6 
#define NODE_VERSION		8 // 7

#define NODE_MAX_RANGE		4096.0f

class idBotNode {
public:

	friend class idBotAI;
	friend class idBotNodeGraph;

							idBotNode();
	virtual					~idBotNode() {}

	// removes a link to a specific node (normally used when the other node is being deleted)
	void					RemoveLink( const idBotNode * node );
	void					AddLink( const idBotNode * node );

	int						GetNum() { return num; }

private:

	struct botLink_t {
		const idBotNode * node;
		float cost;				// basically the length
	};

	int						num;
	int						flags; 
	bool					active;
	float					radius;
	playerTeamTypes_t		team;
	idVec3					origin;
	idStr					name;
	idList< botLink_t >		links;
};

enum {
	NODE_ALL_TYPES = 0,
	NODE_HUSKY_ONLY = 1,
	NODE_GROUND = 2,
	NODE_WATER = 16,
	NODE_GROUND_AND_WATER = 20
};

class idBotNodeGraph {
public:
	// clears the node graph
	void					Clear();

	int						Num();

	// loads nodes from a file
	void					LoadNodes( const char * filename );

	// saves nodes to a file
	void					SaveNodes( const char * filename );

	// draws the nodes for debugging
	void					DrawNodes();

	// adds a new node and expands the radius up to maxRadius or the closest wall edge
	idBotNode *				AddNode( const idVec3 & origin, float maxRadius );

	// removes a node and removes all links to it
	void					DeleteNode( idBotNode * node );

	// returns the node nearest to the origin
	idBotNode *				GetNearestNode( const idMat3& axis, const idVec3& origin, const playerTeamTypes_t team, const moveDirections_t moveDir = NULL_DIR, bool reachableOnly = false, bool activeOnly = false, bool closeNodeOnly = false, idBotNode* ignoreNode = NULL, int vehicleFlags = NODE_ALL_TYPES );

	bool					ActiveVehicleNodeNearby( const idVec3& p, float range );

	// returns a list of nodes (in botPathList) to get from start to end
	void					CreateNodePath( const struct clientInfo_t *botInfo, const idBotNode *start, const idBotNode *end, idList<idBotNode::botLink_t> & botPathList, int vehicleFlags = NODE_ALL_TYPES ) const;

	// (de)activates all nodes matching the group number
	void					ActivateNodes( const char * nodeName, bool activate );

	void					SetNodeTeam( const char * nodeName, const playerTeamTypes_t playerTeam );

	// returns the nearest node for editing
	static idBotNode *		GetNearestEditNode();

	// various commands for editing the nodes
	static void				Cmd_NodeAdd_f( const idCmdArgs &args );
	static void				Cmd_NodeDel_f( const idCmdArgs &args );
	static void				Cmd_NodeName_f( const idCmdArgs &args );
	static void				Cmd_NodeTeam_f( const idCmdArgs &args );
	static void				Cmd_NodeRadius_f( const idCmdArgs &args );
	static void				Cmd_NodeView_f( const idCmdArgs &args );
	static void				Cmd_SaveNodes_f( const idCmdArgs &args );
	static void				Cmd_NodeActive_f( const idCmdArgs &args );
	static void				Cmd_NodeFlags_f( const idCmdArgs &args );
	static void				Cmd_CreateLink_f( const idCmdArgs &args );
	static void				Cmd_GenerateNodes_f( const idCmdArgs &args );
	static void				Cmd_ClearNodes_f( const idCmdArgs &args );
	static void				Cmd_GenerateFromBotActions_f( const idCmdArgs &args );

private:
	// returns the straight line distance from node1 to node2
	float					GetNodeDistance( const idBotNode * node1, const idBotNode * node2 ) const;

private:
	idList< idBotNode * >	nodes;
	static idBotNode *		lastEditNode;
};


#endif // __BOTAI_NODES_H__
