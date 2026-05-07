using VoxelLib.Primitives;

public static class Globals
{
    public const int X_CHUNK_SIZE = 16;
    public const int Z_CHUNK_SIZE = 16;
    public const int CHUNK_RADIUS = 32;
    public const int MAX_X_CHUNK = 32;
    public const int MAX_Z_CHUNK = 32;
    
    public const float LIMITING_STATIC_FRICTION_COEFFICIENT = 0.5f;
    public const float KINETIC_FRICTION_COEFFICIENT = 0.999999F;
    public const float FORWARD_ACCELERATION_FORCE = 1000;
    public const float SIDE_ACCELERATION_FORCE = 200;
    public const float PLAYER_MAX_SPEED = 5;

    
    public const float YAW_ROT_SPEED = 50;
    public const float PITCH_ROT_SPEED = 20;
    
    public const float PLAYER_HEIGHT_SCALE = 5; // The actual player height in Unity Coordinates
    public const float ARBITRARY_AVATAR_UNIT_HEIGHT = 10.5f; // The sum of all Y components in the avatar
    
    public const float TOP_HEIGHT = ARBITRARY_AVATAR_UNIT_HEIGHT / PLAYER_HEIGHT_SCALE;
    
    public const float PLAYER_HEIGHT = 4.5f * PLAYER_HEIGHT_SCALE / ARBITRARY_AVATAR_UNIT_HEIGHT; // Refers to pelvis
    
    public static readonly Vector3F SCALED_SIZE = new(
        PLAYER_HEIGHT_SCALE / ARBITRARY_AVATAR_UNIT_HEIGHT,
        PLAYER_HEIGHT_SCALE / ARBITRARY_AVATAR_UNIT_HEIGHT,
        PLAYER_HEIGHT_SCALE / ARBITRARY_AVATAR_UNIT_HEIGHT);
    
    
    public const int SETTLEMENT_VORONOI_SEED = 6;
    
    public const float L_SYSTEM_SCALE_3D = 0.1f;
    public const float L_SYSTEM_SCALE_2D = 0.01f;
    public const int L_SYSTEM_GRANULARITY = 50;
    

    
    public const float CITY_SCALE = 8.0f;
    public const int MAX_BUILDING_HEIGHT = 50;
    public const int ROAD_RADIUS = 2;
    public const float START_HEALTH = 100;
    public const float BASE_DAMAGE = 20;
    public const float ATTACK_RANGE = 5;
    public const float ATTACK_CD = 5;
    public const float NPC_PLAYER_ACTIVE_DISTANCE_SQUARED = 1600;
    public const float UPDATE_ACTIVE_CD = 5;

    

    public static readonly Vector3F GRAVITY_ACCELERATION = new(0, -9.8F, 0);
    public static int TERRAIN_HEIGHT_RANGE = 16;
    public static int TEAMS = 2;
    public static readonly int MAX_X = X_CHUNK_SIZE * MAX_X_CHUNK;
    public static readonly int MAX_Z = Z_CHUNK_SIZE * MAX_Z_CHUNK;

    public const string CITY_NAME = "1.9 c city";
}