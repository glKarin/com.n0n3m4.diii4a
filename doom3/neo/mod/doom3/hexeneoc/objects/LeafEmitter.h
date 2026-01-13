#ifndef __LEAFEMITTER_H__
#define __LEAFEMITTER_H__

class idEntity_LeafEmitter : idEntity {
public:
	CLASS_PROTOTYPE( idEntity_LeafEmitter );
	void	Spawn();
	void	Think();

private:
	float	nextLeaf;
	float	interval;
	float	maxLeaf;
	idDict	leaf;
};

#endif // __LEAFEMITTER_H__
